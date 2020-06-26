#ifndef __CUSTOM_H__
#define __CUSTOM_H__

#include "tcl.h"

int delayCmd(ClientData clientData, Tcl_Interp *interp, int argc,  char *argv[]);


void task_led_systickintr_handler(void);
void task_led_init(void);
int LedCmd(ClientData clientData, Tcl_Interp *interp, int argc,  char *argv[]);

#endif