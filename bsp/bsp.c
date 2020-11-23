#include "bsp.h"
#include "plic_driver.h"

#ifdef configUART16550_BASE
#include "uart16550.h"
#endif

plic_instance_t Plic;

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
