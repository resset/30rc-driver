/*
    30RC - Copyright (C) 2017 Mateusz Tomaszkiewicz

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
#include "chprintf.h"
#include "shell.h"

#include <stdlib.h>

#include "shell_main.h"

#define LINE_CLK 10
#define LINE_DIR 11
#define LINE_ENA 12

#define SHELL_WA_SIZE THD_WORKING_AREA_SIZE(2048)

static thread_t *shelltp = NULL;

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

/*
 * Shell exit event.
 */
static void ShellHandler(eventid_t id) {

  (void)id;
  if (chThdTerminatedX(shelltp)) {
    chThdWait(shelltp);                 /* Returning memory to heap.        */
    shelltp = NULL;
  }
}

THD_WORKING_AREA(waShell, 128);
THD_FUNCTION(thShell, arg) {
  (void)arg;

  static const evhandler_t evhndl[] = {
    ShellHandler
  };
  event_listener_t el0;

  shellInit();

  chEvtRegister(&shell_terminated, &el0, 0);
  while (true) {
    if (!shelltp) {
      shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                    "shell", NORMALPRIO + 1,
                                    shellThread, (void *)&shell_cfg1);
    }
    chEvtDispatch(evhndl, chEvtWaitOneTimeout(ALL_EVENTS, MS2ST(500)));
  }
}

void shell_init(void)
{
  chThdCreateStatic(waShell, sizeof(waShell),
                    NORMALPRIO, thShell, NULL);
}
