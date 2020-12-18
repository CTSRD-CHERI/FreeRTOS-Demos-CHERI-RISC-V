#include "bsp.h"
#include "plic_driver.h"

#ifdef configUART16550_BASE
#include "uart16550.h"
#endif

plic_instance_t Plic;

#ifdef __CHERI_PURE_CAPABILITY__
#include <stdint.h>
#include <rtl/rtl-freertos-compartments.h>
#include <cheri/cheri-utility.h>
#include "portmacro.h"

#ifdef __CHERI_PURE_CAPABILITY__
static void inter_compartment_call(uintptr_t *exception_frame, ptraddr_t mepc) {
  uint32_t *instruction;
  uint8_t  code_reg_num, data_reg_num;
  uint32_t *mepcc = (uint32_t *) *(exception_frame);
  mepcc -= 1; // portASM has already stepped into the next isntruction;

  instruction = mepcc;

  // Decode the code/data pair passed to ccall
  code_reg_num = (*instruction >> 15) & 0x1f;
  data_reg_num = (*instruction >> 20) & 0x1f;

  uint32_t cjalr_match =  ((0x7f << 25) | (0xc << 20) | (0x1 << 7) | 0x5b);

  /* Only support handling cjalr with sealed caps (inter-compartment calls) */
  if ((*instruction & cjalr_match) != cjalr_match) {
    printf("Instruction does not match cjalr\n");
    _exit(-1);
  }

  // Get the callee CompID (its otype)
  size_t otype = __builtin_cheri_type_get(*(exception_frame + code_reg_num));

  void **captable = rtl_cherifreertos_compartment_get_captable(otype);

  xCOMPARTMENT_RET ret = xTaskRunCompartment(cheri_unseal_cap(*(exception_frame + code_reg_num)),
                    captable,
                    exception_frame + 10,
                    otype);

  // Save the return registers in the context.
  // FIXME: Some checks might be done here to check of the compartment traps and
  // accordingly take some different action rather than just returning
  *(exception_frame + 10) = ret.ca0;
}

#endif

static UBaseType_t cheri_exception_handler(uintptr_t *exception_frame)
{
#ifdef __CHERI_PURE_CAPABILITY__
  size_t cause = 0;
  size_t epc = 0;
  size_t cheri_cause;

  asm volatile ("csrr %0, mcause" : "=r"(cause)::);
  asm volatile ("csrr %0, mepc" : "=r"(epc)::);

  size_t ccsr = 0;
  asm volatile ("csrr %0, mccsr" : "=r"(ccsr)::);

  uint8_t reg_num = (uint8_t) ((ccsr >> 10) & 0x1f);
  int is_scr = ((ccsr >> 15) & 0x1);
  cheri_cause = (unsigned) ((ccsr >> 5) & 0x1f);


  // ccall
  if (cheri_cause == 0x19) {
    inter_compartment_call(exception_frame, epc);
    return 0;
  }

  // Sealed fault
  if (cheri_cause == 0x3) {
    inter_compartment_call(exception_frame, epc);
    return 0;
  }

  for(int i = 0; i < 35; i++) {
    printf("x%i ", i); cheri_print_cap(*(exception_frame + i));
  }

  printf("mepc = 0x%lx\n", epc);
  printf("TRAP: CCSR = 0x%lx (cause: %x reg: %u : scr: %u)\n",
               ccsr,
               cheri_cause,
               reg_num, is_scr);
#endif
  while(1);
}

static UBaseType_t default_exception_handler(uintptr_t *exception_frame)
{
  size_t cause = 0;
  size_t epc = 0;
  asm volatile ("csrr %0, mcause" : "=r"(cause)::);
  asm volatile ("csrr %0, mepc" : "=r"(epc)::);
  printf("mcause = %u\n", cause);
  printf("mepc = %llx\n", epc);
  while(1);
}
#endif
/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/
/**
 *  Prepare haredware to run the demo.
 */
void prvSetupHardware(void) {
  // Resets PLIC, threshold 0, nothing enabled

#ifdef PLATFORM_QEMU_VIRT
  PLIC_init(&Plic, PLIC_BASE_ADDR, PLIC_NUM_SOURCES, PLIC_NUM_PRIORITIES);
#endif

#ifdef configUART16550_BASE
  uart16550_init(configUART16550_BASE);
#endif

#ifdef __CHERI_PURE_CAPABILITY__
  /* Setup an exception handler for CHERI */
  vPortSetExceptionHandler(0x1c, cheri_exception_handler);
#endif
}

#ifndef PLATFORM_QEMU_VIRT
__attribute__((weak)) BaseType_t xNetworkInterfaceInitialise( void ) {
  printf("xNetworkInterfaceInitialise is not implemented, No NIC backend driver\n");
  return pdPASS;
}

__attribute__((weak))
xNetworkInterfaceOutput( void * const pxNetworkBuffer, BaseType_t xReleaseAfterSend ) {
  printf("xNetworkInterfaceOutput is not implemented, No NIC backend driver\n");
  return pdPASS;
}
#endif

/**
 * Define an external interrupt handler
 * cause = 0x8...000000b == Machine external interrupt
 */
void external_interrupt_handler(uint32_t cause) {
  configASSERT((cause << 1 ) == (0xb * 2));

  plic_source source_id = PLIC_claim_interrupt(&Plic);

  if ((source_id >= 1) && (source_id < PLIC_NUM_INTERRUPTS)) {
    Plic.HandlerTable[source_id].Handler(Plic.HandlerTable[source_id].CallBackRef);
  }

  // clear interrupt
  PLIC_complete_interrupt(&Plic, source_id);
}
