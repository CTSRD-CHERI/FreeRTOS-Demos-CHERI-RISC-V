/* See LICENSE for license details. */

#include <string.h>
#include <FreeRTOSConfig.h>
#include "uart16550.h"

#ifdef __CHERI_PURE_CAPABILITY__
#include <cheri/cheri-utility.h>
#endif /* __CHERI_PURE_CAPABILITY__ */

#if configUART16550_REGSHIFT == 1
    volatile uint8_t* uart16550;
    typedef volatile uint8_t* uart_mmio_t;
#elif configUART16550_REGSHIFT == 2
    volatile uint32_t* uart16550;
    typedef volatile uint32_t* uart_mmio_t;
#else
    #error "Unsupported uart reg_shift value"
#endif

#define UART_REG_QUEUE        0
#define UART_REG_LINESTAT     ( 5 )
#define UART_REG_STATUS_RX    ( 0x01 )
#define UART_REG_STATUS_TX    ( 0x20 )

static inline uint32_t bswap( uint32_t x )
{
    uint32_t y = ( x & 0x00FF00FF ) << 8 | ( x & 0xFF00FF00 ) >> 8;
    uint32_t z = ( y & 0x0000FFFF ) << 16 | ( y & 0xFFFF0000 ) >> 16;

    return z;
}

void uart16550_putchar( uint8_t ch )
{
    while( ( uart16550[ UART_REG_LINESTAT ] & UART_REG_STATUS_TX ) == 0 )
    {
    }

    uart16550[ UART_REG_QUEUE ] = ch;
}

int uart16550_getchar()
{
    if( uart16550[ UART_REG_LINESTAT ] & UART_REG_STATUS_RX )
    {
        return uart16550[ UART_REG_QUEUE ];
    }

    return -1;
}

int uart16550_txbuffer( uint8_t * ptr,
                        int len )
{
    for( int i = 0; i < len; i++ )
    {
        uart16550_putchar( ptr[ i ] );

        if( ptr[ i ] == '\n' )
        {
            uart16550_putchar( '\r' );
        }
    }

    return len;
}

void uart16550_init( intptr_t base )
{
    uart16550 = ( uart_mmio_t ) base;

    uint32_t divisor;
    divisor = configPERIPH_CLOCK_HZ / ( 16 * configUART16550_BAUD );
    #ifdef __CHERI_PURE_CAPABILITY__
        uart16550 =
            ( uart_mmio_t ) cheri_build_data_cap((ptraddr_t) base,
                0x1000,
                __CHERI_CAP_PERMISSION_PERMIT_LOAD__ |
                __CHERI_CAP_PERMISSION_PERMIT_STORE__);
    #endif /* __CHERI_PURE_CAPABILITY__ */

    /* http://wiki.osdev.org/Serial_Ports */
    uart16550[ 1 ] = 0x00;                    /* Disable all interrupts */
    uart16550[ 3 ] = 0x80;                    /* Enable DLAB (set baud rate divisor) */
    uart16550[ 0 ] = divisor & 0xff;          /* Set divisor (lo byte) baud */
    uart16550[ 1 ] = ( divisor >> 8 ) & 0xff; /* Set divisor to (hi byte) baud */
    uart16550[ 3 ] = 0x03;                    /* 8 bits, no parity, one stop bit */
    uart16550[ 2 ] = 0xC7;                    /* Enable FIFO, clear them, with 14-byte threshold */
}
