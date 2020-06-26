#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "inc/tm4c123gh6pm.h"
#include "uart_stream.h"
#include "delay.h"
#include "efs.h"
#include "custom.h"
#include "systick.h"
#include "gpio.h"
#include "tcl.h"

void virtual_efs_init(void)
{
	//create emulated fs
    Efs_FsHdr_t fs_hdr;
    fs_hdr.MagicKey             = 0x0EF5;
    fs_hdr.Checksum             = 0xABCD;
    fs_hdr.MaxFileCount         = 4;
    fs_hdr.PageSize             = 64;
    fs_hdr.PageCount            = 32;
    fs_hdr.BitmapSize           = 4;
    fs_hdr.DataStartPage        = 2;

    EfsFsCreate(fs_hdr);


    //load FS
    EfsFsLoad();
}

void virtual_tcl_fs_init(void)
{
	#define CAT_PROC "proc cat {file} {\n"\
	" set fp [open $file r]\n"\
	" puts [read $fp] \n"\
	" close $fp \n"\
	"}\n"
	
	#define EDIT_PROC "proc edit {file} {\n"\
	" set fp [open $file w]\n"\
	" while {1} {\n"\
	"  if {[gets stdin line]} {\n"\
	"   puts $fp $line\n"\
	"   } else {\n"\
	"   close $fp\n"\
	"   break\n"\
	"  }\n"\
	" }\n"\
	"}"
	
	FILE *fp;
	fp = fopen("init.tcl","w+");
    if(fp==NULL)
        while(1);
	fprintf(fp,"puts \"Welcome to Embedded TCL\" \n");
	fprintf(fp,CAT_PROC);
	fprintf(fp,EDIT_PROC);
	fclose(fp);
	
	#define TEST_TCL "for {set i 0} {$i < 10} {incr i} {\n"\
	" puts $i\n"\
	" led blue on\n"\
	" delay 500\n"\
	" led blue off\n"\
	" delay 500 \n"\
	"}"
	
	fp = fopen("test.tcl","w+");
    if(fp==NULL)
        while(1);
	fprintf(fp,TEST_TCL);
	fclose(fp);
}


/****************************************Async Handlers*********************************/
Tcl_AsyncHandler tcl_sigkill;
int tcl_sigkill_handler(ClientData clientData,Tcl_Interp *interp, int code)
{
	return TCL_BREAK;
}

Tcl_AsyncHandler tcl_async_sw2;
extern char trigger[1][100];
int tcl_sw2_handler(ClientData clientData,Tcl_Interp *interp, int code)
{
	if(trigger[0][0]!='\0')
		return Tcl_Eval(interp, trigger[0]);
	else
		return TCL_OK;
}


/*************************************Main************************************************/
int main(void)
{
	//peripherals init
	systick_init(1000);
	gpio_portf_enable();
	gpio_portf_intr_enable(SW1_BIT, 0, 0, 0);
	gpio_portf_intr_enable(SW2_BIT, 0, 0, 0);
	nvic_set_priority(&NVIC_PRI7_R, 2, 5);
    nvic_enable_irq(30);
	
	//emulated file system init
	virtual_efs_init();
	
	//create initial files in virtual FS
	virtual_tcl_fs_init();
	
	//register Async Handlers
	tcl_sigkill = Tcl_AsyncCreate(tcl_sigkill_handler,NULL);
	tcl_async_sw2 = Tcl_AsyncCreate(tcl_sw2_handler,NULL);
	
	//init internal system blocks
    task_led_init();
	
	//init & run tcl console
	tcl_main();
	
	return 0;
}

/******************************ISR Handlers****************************************/
extern struct event_ctl_s event_ctl[1];
volatile uint32_t ticks = 0;
void systick_handler(void)
{
	ticks++;
	task_led_systickintr_handler();
	if(event_ctl[0].count_ms != 0)
	{
		if(ticks % event_ctl[0].count_ms == 0)
		{
			Tcl_AsyncMark(event_ctl[0].async);
		}
	}
}

void portf_handler(void)
{
	if (check(GPIO_PORTF_RIS_R, SW1_BIT))
    {
        //clear interrupt source
        sbit(GPIO_PORTF_ICR_R, SW1_BIT);

        Tcl_AsyncMark(tcl_sigkill);

        //clear interrupt source
        sbit(GPIO_PORTF_ICR_R, SW1_BIT);
        
    }
	
	if (check(GPIO_PORTF_RIS_R, SW2_BIT))
    {
        //clear interrupt source
        sbit(GPIO_PORTF_ICR_R, SW2_BIT);

        Tcl_AsyncMark(tcl_async_sw2);

        //clear interrupt source
        sbit(GPIO_PORTF_ICR_R, SW2_BIT);
        
    }
	
}

extern Tcl_Interp *interp;
int tcl_uart_async_handler(void)
{
	if (tcl_AsyncReady) 
	{
		return Tcl_Eval(interp, "");
	}
}