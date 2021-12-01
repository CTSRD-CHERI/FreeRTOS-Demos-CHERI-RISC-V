#include <stdint.h>
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "bsp.h"
#include "portmacro.h"
#include "portstatcounters.h"
#include "plic_driver.h"

#if PLATFORM_GFE
    #include "iic.h"
#endif

#ifdef configUART16550_BASE
    #include "uart16550.h"
#endif

#if (configMPU_COMPARTMENTALIZATION == 1 || configCHERI_COMPARTMENTALIZATION == 1)
    #include <rtl/rtl-freertos-compartments.h>
#endif


plic_instance_t Plic;

#define CHERI_COMPARTMENT_FAIL (-28)

#if __CHERI_PURE_CAPABILITY__
    #include <cheri/cheri-utility.h>

    #if configCHERI_VERBOSE_FAULT_INFO
        const char* cheri_faults[] = {
            "None",                                     // 0
            "Length Violation",                         // 1
            "Untagged Capability",                      // 2
            "Sealed",                                   // 3
            "Type",                                     // 4
            "Reserved",                                 // 5
            "Reserved",                                 // 6
            "Reserved",                                 // 7
            "SW Permission",                            // 8
            "Reserved",                                 // 9
            "Representability",                         // 0xa
            "Unaligned Base",                           // 0xb
            "Reserved",                                 // 0xc
            "Reserved",                                 // 0xd
            "Reserved",                                 // 0xe
            "Reserved",                                 // 0xf
            "Global",                                   // 0x10
            "Execute Permission",                       // 0x11
            "Load Permission",                          // 0x12
            "Store Permission",                         // 0x13
            "Load Capability Permission",               // 0x14
            "Store Capability Permission",              // 0x15
            "Store Local Capability Permission",        // 0x16
            "Seal Permission",                          // 0x17
            "Access System Registers Permission",       // 0x18
            "CInvoke Permission",                       // 0x19
            "Access CInvoke IDC",                       // 0x1a
            "Unseal Permission",                        // 0x1b
            "Set CID Permission",                       // 0x1c
            "Reserved",                                 // 0x1d
            "Reserved",                                 // 0x1e
            "Reserved"                                  // 0x1f
        };
    #endif

    #if configCHERI_STACK_TRACE
        static void backtrace(void* pc, void* sp, void* ra, size_t xCompID) {
            printf( "\033[0;33m" );
            printf("<STACK TRACE START>\n");
            rtl_cherifreertos_compartment_backtrace(pc, sp, ra, xCompID);
            printf( "\033[0;33m" );
            printf("<STACK TRACE END>\n");
            printf( "\033[0m" );
        }
    #endif

    #if configCHERI_COMPARTMENTALIZATION
        static void inter_compartment_call( uintptr_t * exception_frame,
                                            ptraddr_t mepc )
        {
            uint32_t * instruction;
            uint8_t code_reg_num, data_reg_num;
            uint32_t * mepcc = ( uint32_t * ) *( exception_frame );

            mepcc -= 1; /* portASM has already stepped into the next isntruction; */

            instruction = mepcc;

            /* Decode the code/data pair passed to ccall */
            code_reg_num = ( *instruction >> 15 ) & 0x1f;
            data_reg_num = ( *instruction >> 20 ) & 0x1f;

            uint32_t cjalr_match = ( ( 0x7f << 25 ) | ( 0xc << 20 ) | ( 0x1 << 7 ) | 0x5b );

            /* Only support handling cjalr with sealed caps (inter-compartment calls) */
            if( ( *instruction & cjalr_match ) != cjalr_match )
            {
                printf( "Instruction does not match cjalr\n" );
                exit( -1 );
            }

            /* Get the callee CompID (its otype) */
            size_t otype = __builtin_cheri_type_get( *( exception_frame + code_reg_num ) );

            void ** captable = rtl_cherifreertos_compartment_get_captable( otype );

            xCOMPARTMENT_RET ret = xTaskRunCompartment( cheri_unseal_cap( ( void * ) *( exception_frame + code_reg_num ) ),
                                                        captable,
                                                        ( xCOMPARTMENT_ARGS * ) (exception_frame + 10),
                                                        otype );

            /* Save the return registers in the context. */
            /* FIXME: Some checks might be done here to check of the compartment traps and */
            /* accordingly take some different action rather than just returning */
            *( exception_frame + 10 ) = ( uintptr_t ) ret.ca0;
        }

    #endif /* ifdef __CHERI_PURE_CAPABILITY__ */

    __attribute__((section(".text.fast"))) static uint32_t default_exception_handler( uintptr_t * exception_frame )
    {
            BaseType_t pxHigherPriorityTaskWoken = 0;
        #ifdef __CHERI_PURE_CAPABILITY__
            size_t cause = 0;
            size_t epc = 0;
            size_t cheri_cause;
            void * mepcc;
            #if configCHERI_COMPARTMENTALIZATION
                size_t xCompID = -1;
                rtems_rtl_obj * obj = NULL;
            #endif

            asm volatile ( "csrr %0, mcause" : "=r" ( cause )::);
            asm volatile ( "csrr %0, mepc" : "=r" ( epc )::);
            asm volatile ( "cspecialr %0, mepcc" : "=C" ( mepcc )::);

            size_t ccsr = 0;
            asm volatile ( "csrr %0, mtval" : "=r" ( ccsr )::);

            uint8_t reg_num = ( uint8_t ) ( ( ccsr >> 5 ) & 0x1f );
            int is_scr = ( ( ccsr >> 10 ) & 0x1 );
            cheri_cause = ( unsigned ) ( ( ccsr ) & 0x1f );

            #if configCHERI_COMPARTMENTALIZATION
            /* ccall */
            if( cheri_cause == 0x19 )
            {
                inter_compartment_call( exception_frame, epc );
                return 0;
            }

            /* Sealed fault */
            if( cheri_cause == 0x3 )
            {
                inter_compartment_call( exception_frame, epc );
                return 0;
            }
            #endif

            #if (DEBUG || configCHERI_VERBOSE_FAULT_INFO)
                for( int i = 0; i < 35; i++ )
                {
                    printf( "x%i ", i );
                    cheri_print_cap( *( exception_frame + i ) );
                }

                printf( "mepc = 0x%lx\n", epc );
                printf( "mepcc -> " );
                cheri_print_cap( mepcc );
                printf( "TRAP: MTVAL = 0x%lx (cause: 0x%x reg: %u : scr: %u)\n",
                        ccsr,
                        cheri_cause,
                        reg_num, is_scr );
            #endif

            #if configCHERI_COMPARTMENTALIZATION
                xCompID = xPortGetCurrentCompartmentID();
            #if configCHERI_COMPARTMENTALIZATION_MODE == 1
                obj = rtl_cherifreertos_compartment_get_obj( xCompID );

                if( obj != NULL )
                {
                    void * ret = xPortGetCurrentCompartmentReturn();
                    #if configCHERI_VERBOSE_FAULT_INFO
                        printf( "\033[0;31m" );
                        printf( "<<<< %s Fault at 0x%x in Task: %s: Compartment #%d: %s:%s\n", cheri_faults[cheri_cause], (size_t) mepcc, pcTaskGetName( NULL ), xCompID, obj->aname, obj->oname );
                        printf( "\033[0m" );
                    #endif

                    #if configCHERI_STACK_TRACE
                        void* sp = ( void * ) *( exception_frame + 2 );
                        void* ra = ( void * ) *( exception_frame + 1 );
                        backtrace(mepcc, sp, ra, xCompID);
                    #endif

                    pxHigherPriorityTaskWoken = rtl_cherifreertos_compartment_faultHandler(xCompID);

                    /* Caller compartment return */
                    *( exception_frame ) = ( uintptr_t ) ret;
                    *( exception_frame + 10) = ( uintptr_t ) CHERI_COMPARTMENT_FAIL;
                    return 0;
                }
            #elif configCHERI_COMPARTMENTALIZATION_MODE == 2
                rtems_rtl_archive* archive = rtl_cherifreertos_compartment_get_archive( xCompID );

                if( archive != NULL )
                {
                    void * ret = xPortGetCurrentCompartmentReturn();

                    #if configCHERI_VERBOSE_FAULT_INFO
                        printf( "\033[0;31m" );
                        printf( "<<<< %s Fault in Task: %s: Compartment #%d: %s\n", cheri_faults[cheri_cause], pcTaskGetName( NULL ), xCompID, archive->name );
                        printf( "\033[0m" );
                    #endif

                    pxHigherPriorityTaskWoken = rtl_cherifreertos_compartment_faultHandler(xCompID);

                    /* Caller compartment return */
                    *( exception_frame ) = ( uintptr_t ) ret;
                    *( exception_frame + 10) = ( uintptr_t ) CHERI_COMPARTMENT_FAIL;
                    return pxHigherPriorityTaskWoken;
                }

            #endif
            #endif /* if configCHERI_COMPARTMENTALIZATION */
        #endif /* ifdef __CHERI_PURE_CAPABILITY__ */

        while( 1 )
        {
        }
    }
#else
    __attribute__((section(".text.fast"))) static uint32_t default_exception_handler( uintptr_t * exception_frame )
    {
            BaseType_t pxHigherPriorityTaskWoken = 0;
            size_t cause = 0;
            size_t epc = 0;
            size_t mtval = 0;
            #if configMPU_COMPARTMENTALIZATION
                size_t xCompID = -1;
                rtems_rtl_obj * obj = NULL;
            #endif

            asm volatile ( "csrr %0, mcause" : "=r" ( cause )::);
            asm volatile ( "csrr %0, mepc" : "=r" ( epc )::);
            asm volatile ( "csrr %0, mtval" : "=r" ( mtval )::);


            #if (DEBUG)
                printf( "mcause -> 0x%zx\n", cause );
                printf( "mepc -> 0x%zx\n", epc );
                printf( "mtval -> 0x%zu\n\n", mtval );

                printf( "GPRs:\n" );
                for( int i = 0; i < 32; i++ )
                {
                    printf( "x%i -> 0x%zx\n", i, *( exception_frame + i ));
                }

            #endif

            #if configMPU_COMPARTMENTALIZATION
                xCompID = xPortGetCurrentCompartmentID();
            #if configMPU_COMPARTMENTALIZATION_MODE == 1
                obj = rtl_cherifreertos_compartment_get_obj( xCompID );

                if( obj != NULL )
                {
                    void * ret = xPortGetCurrentCompartmentReturn();

                    #if (DEBUG)
                        printf( "<<<< Fault in Task %s: Compartment #%d: %s\n", pcTaskGetName( NULL ), xCompID, obj->oname );
                    #endif

                    pxHigherPriorityTaskWoken = rtl_cherifreertos_compartment_faultHandler(xCompID);

                    /* Caller compartment return */
                    *( exception_frame ) = ( uintptr_t ) ret;
                    *( exception_frame + 7) = ( uintptr_t ) -1;
                    return 0;
                }
            #elif configMPU_COMPARTMENTALIZATION_MODE == 2
                rtems_rtl_archive* archive = rtl_cherifreertos_compartment_get_archive( xCompID );

                if( archive != NULL )
                {
                    void * ret = xPortGetCurrentCompartmentReturn();

                    #if (DEBUG)
                        printf( "<<<< Fault in Task %s: Compartment #%d: %s\n", pcTaskGetName( NULL ), xCompID, archive->name );
                    #endif

                    pxHigherPriorityTaskWoken = rtl_cherifreertos_compartment_faultHandler(xCompID);

                    /* Caller compartment return */
                    *( exception_frame ) = ( uintptr_t ) ret;
                    *( exception_frame + 7) = ( uintptr_t ) -1;
                    return pxHigherPriorityTaskWoken;
                }

            #endif
            #endif

        while( 1 )
        {
        }
    }
#endif /* ifdef __CHERI_PURE_CAPABILITY__ */
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

/**
 *  Prepare haredware to run the demo.
 */
void prvSetupHardware( void )
{
    /* Resets PLIC, threshold 0, nothing enabled */

    #if PLATFORM_QEMU_VIRT || PLATFORM_FETT || PLATFORM_GFE
        PLIC_init( &Plic, PLIC_BASE_ADDR, PLIC_NUM_SOURCES, PLIC_NUM_PRIORITIES );
    #endif

    #ifdef configUART16550_BASE
        uart16550_init( configUART16550_BASE );
    #endif

    #if PLATFORM_GFE
        configASSERT(BSP_USE_DMA);
        PLIC_set_priority(&Plic, PLIC_SOURCE_ETH, PLIC_PRIORITY_ETH);
        PLIC_set_priority(&Plic, PLIC_SOURCE_DMA_MM2S, PLIC_PRIORITY_DMA_MM2S);
        PLIC_set_priority(&Plic, PLIC_SOURCE_DMA_S2MM, PLIC_PRIORITY_DMA_S2MM);
        #if BSP_USE_IIC0
            PLIC_set_priority(&Plic, PLIC_SOURCE_IIC0, PLIC_PRIORITY_IIC0);
            iic0_init();
        #endif
    #endif

    #if ((DEBUG) || (configCHERI_COMPARTMENTALIZATION == 1) || (configMPU_COMPARTMENTALIZATION == 1))
        /* Setup an exception handler for CHERI */
        for (int i = 0; i < 64; i++)
            vPortSetExceptionHandler( i, default_exception_handler );
    #endif

    #if configPORT_HAS_HPM_COUNTERS
        portCountersInit();
    #endif
}

#if !(PLATFORM_QEMU_VIRT || PLATFORM_FETT || PLATFORM_GFE)
__attribute__( ( weak ) ) BaseType_t xNetworkInterfaceInitialise( void )
{
    printf( "xNetworkInterfaceInitialise is not implemented, No NIC backend driver\n" );
    return pdPASS;
}

__attribute__( ( weak ) )
xNetworkInterfaceOutput( void * const pxNetworkBuffer, BaseType_t xReleaseAfterSend )
{
    printf( "xNetworkInterfaceOutput is not implemented, No NIC backend driver\n" );
    return pdPASS;
}
#endif

/**
 * Define an external interrupt handler
 * cause = 0x8...000000b == Machine external interrupt
 */
__attribute__((section(".text.fast"))) BaseType_t external_interrupt_handler( UBaseType_t cause )
{
    BaseType_t pxHigherPriorityTaskWoken = 0;

    configASSERT( ( cause << 1 ) == ( 0xb * 2 ) );

    plic_source source_id = PLIC_claim_interrupt( &Plic );

    if( ( source_id >= 1 ) && ( source_id < PLIC_NUM_INTERRUPTS ) )
    {
        pxHigherPriorityTaskWoken = Plic.HandlerTable[ source_id ].Handler( Plic.HandlerTable[ source_id ].CallBackRef );
    }

    /* clear interrupt */
    PLIC_complete_interrupt( &Plic, source_id );
    return pxHigherPriorityTaskWoken;
}
