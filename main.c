#include "ch.h"
#include "hal.h"
#include "test.h"

#include "chprintf.h"
#include "shell.h"

#include <stdlib.h>

#include "shell_main.h"

#define LINE_CLK 10
#define LINE_DIR 11
#define LINE_ENA 12

static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {
  systime_t time;                   /* Next deadline.*/

  (void)arg;
  chRegSetThreadName("reader");

  /* Reader thread loop.*/
  time = chVTGetSystemTime();
  while (true) {
    /* Waiting until the next 250 milliseconds time interval.*/
    chThdSleepUntil(time += MS2ST(100));
  }
}

/*===========================================================================*/
/* Initialization and main thread.                                           */
/*===========================================================================*/

/*
 * Application entry point.
 */
int main(void) {
  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Activates the serial driver 2 using the driver default configuration.
   * PA2(TX) and PA3(RX) are routed to USART2.
   */
  sdStart(&SD2, NULL);
  palSetPadMode(GPIOA, 2, PAL_MODE_ALTERNATE(7));
  palSetPadMode(GPIOA, 3, PAL_MODE_ALTERNATE(7));

  /*
   * Initializes board LEDs.
   */
  palSetPadMode(GPIOD, GPIOD_LED4, PAL_MODE_OUTPUT_PUSHPULL);      /* Green.   */
  palSetPadMode(GPIOD, GPIOD_LED3, PAL_MODE_OUTPUT_PUSHPULL);      /* Orange.  */
  palSetPadMode(GPIOD, GPIOD_LED5, PAL_MODE_OUTPUT_PUSHPULL);      /* Red.     */
  palSetPadMode(GPIOD, GPIOD_LED6, PAL_MODE_OUTPUT_PUSHPULL);      /* Blue.    */

  palSetPadMode(GPIOE, LINE_CLK, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOE, LINE_DIR, PAL_MODE_OUTPUT_PUSHPULL);
  palSetPadMode(GPIOE, LINE_ENA, PAL_MODE_OUTPUT_PUSHPULL);

  palClearPad(GPIOE, LINE_CLK);
  palSetPad(GPIOE, LINE_DIR);
  palClearPad(GPIOE, LINE_ENA);

  palClearPad(GPIOD, GPIOD_LED4);
  palSetPad(GPIOD, GPIOD_LED3);
  palClearPad(GPIOD, GPIOD_LED5);

  chThdCreateStatic(waThread1, sizeof(waThread1),
                    NORMALPRIO + 10, Thread1, NULL);

  chThdCreateStatic(waShell, sizeof(waShell),
                    NORMALPRIO, thShell, NULL);

  while (true) {
    chThdSleepMilliseconds(500);
  }

  return 0;
}
