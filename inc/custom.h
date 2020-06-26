#ifndef __CUSTOM_H__
#define __CUSTOM_H__

#include "tcl.h"

struct event_ctl_s
{
	uint16_t count_ms;
	char cmd[100];
	Tcl_AsyncHandler async;
};
int delayCmd(ClientData clientData, Tcl_Interp *interp, int argc,  char *argv[]);
void task_led_systickintr_handler(void);
void task_led_init(void);
int LedCmd(ClientData clientData, Tcl_Interp *interp, int argc,  char *argv[]);
int systemCmd(ClientData clientData, Tcl_Interp *interp, int argc,  char *argv[]);

#endif