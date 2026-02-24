#include <stdbool.h>
#include "stm32h755xx.h"

volatile bool irq_enabled = true;

/* checked by emulator_uart and emulator_i2c before mocking ISRs */
void HAL_NVIC_DisableIRQ(IRQn_Type IRQn) {irq_enabled = false;}
void HAL_NVIC_EnableIRQ(IRQn_Type IRQn) {irq_enabled = true;}
