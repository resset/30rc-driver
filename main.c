/*
    ChibiOS - Copyright (C) 2006..2015 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "ch.h"
#include "hal.h"
#include "test.h"

#include "chprintf.h"
#include "shell.h"

#include <stdlib.h>

#define LINE_CLK 10
#define LINE_DIR 11
#define LINE_ENA 12

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)
#define TEST_WA_SIZE    THD_WORKING_AREA_SIZE(256)

static void cmd_mem(BaseSequentialStream *chp, int argc, char *argv[]) {
  size_t n, size;

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: mem\r\n");
    return;
  }
  n = chHeapStatus(NULL, &size);
  chprintf(chp, "core free memory : %u bytes\r\n", chCoreGetStatusX());
  chprintf(chp, "heap fragments   : %u\r\n", n);
  chprintf(chp, "heap free total  : %u bytes\r\n", size);
}

static void cmd_threads(BaseSequentialStream *chp, int argc, char *argv[]) {
  static const char *states[] = {CH_STATE_NAMES};
  thread_t *tp;

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: threads\r\n");
    return;
  }
  chprintf(chp, "    addr    stack prio refs     state\r\n");
  tp = chRegFirstThread();
  do {
    chprintf(chp, "%08lx %08lx %4lu %4lu %9s\r\n",
             (uint32_t)tp, (uint32_t)tp->p_ctx.r13,
             (uint32_t)tp->p_prio, (uint32_t)(tp->p_refs - 1),
             states[tp->p_state]);
    tp = chRegNextThread(tp);
  } while (tp != NULL);
}

static void cmd_test(BaseSequentialStream *chp, int argc, char *argv[]) {
  thread_t *tp;

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: test\r\n");
    return;
  }
  tp = chThdCreateFromHeap(NULL, TEST_WA_SIZE, chThdGetPriorityX(),
                           TestThread, chp);
  if (tp == NULL) {
    chprintf(chp, "out of memory\r\n");
    return;
  }
  chThdWait(tp);
}

static void cmd_enable(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  (void)argc;

  palSetPad(GPIOD, GPIOD_LED5);
  palSetPad(GPIOE, LINE_ENA);
  chprintf(chp, "Stepper enabled\r\n");
}

static void cmd_disable(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  (void)argc;

  palClearPad(GPIOD, GPIOD_LED5);
  palClearPad(GPIOE, LINE_ENA);
  chprintf(chp, "Stepper disabled\r\n");
}

static void cmd_left(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  (void)argc;

  palClearPad(GPIOD, GPIOD_LED3);
  palClearPad(GPIOE, LINE_DIR);
  chprintf(chp, "Stepper moves left\r\n");
}

static void cmd_right(BaseSequentialStream *chp, int argc, char *argv[]) {
  (void)argv;
  (void)argc;

  palSetPad(GPIOD, GPIOD_LED3);
  palSetPad(GPIOE, LINE_DIR);
  chprintf(chp, "Stepper moves right\r\n");
}

static void cmd_step(BaseSequentialStream *chp, int argc, char *argv[]) {
  int moves;
  int delay = 5;

  if (argc == 0) {
    moves = 800;
  } else {
    moves = atoi(argv[1]);
  }
  chprintf(chp, "Stepper: %d moves...\r\n", moves);

  for (uint16_t i = 0; i < moves; i++) {
      palClearPad(GPIOD, GPIOD_LED4);
      palClearPad(GPIOE, LINE_CLK);
      chThdSleepMilliseconds(delay);
      palSetPad(GPIOD, GPIOD_LED4);
      palSetPad(GPIOE, LINE_CLK);
      chThdSleepMilliseconds(delay);
      chprintf(chp, "Step: %d\r\n", i);
  }
  palClearPad(GPIOD, GPIOD_LED4);
  palClearPad(GPIOE, LINE_CLK);

  chprintf(chp, "Done.\r\n");
}

static const ShellCommand commands[] = {
  {"mem", cmd_mem},
  {"threads", cmd_threads},
  {"test", cmd_test},

  {"enable", cmd_enable},
  {"disable", cmd_disable},
  {"left", cmd_left},
  {"right", cmd_right},
  {"s", cmd_step},
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SD2,
  commands
};

/*===========================================================================*/
/* Accelerometer related.                                                    */
/*===========================================================================*/

/*
 * This is a periodic thread that reads accelerometer and outputs
 * result to SPI2 and PWM.
 */
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
  thread_t *shelltp = NULL;

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
   * Shell manager initialization.
   */
  shellInit();

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

  /*
   * Creates the example thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1),
                    NORMALPRIO + 10, Thread1, NULL);

  /*
   * Normal main() thread activity.
   */
  while (true) {
    if (!shelltp)
      shelltp = shellCreate(&shell_cfg1, SHELL_WA_SIZE, NORMALPRIO);
    else if (chThdTerminatedX(shelltp)) {
      chThdRelease(shelltp);    /* Recovers memory of the previous shell.   */
      shelltp = NULL;           /* Triggers spawning of a new shell.        */
    }
    chThdSleepMilliseconds(1000);
  }
}
