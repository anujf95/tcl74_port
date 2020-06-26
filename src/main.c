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
	FILE *fp;
	fp = fopen("init.tcl","w+");
    if(fp==NULL)
        while(1);
	fprintf(fp,"puts \"Welcome to Embedded TCL\" \n");
	fprintf(fp,"proc cat {file} {\n set fp [open $file r]\n puts [read $fp] \n close $fp \n}\n");
	fprintf(fp,"proc edit {file} {\n set fp [open $file w]\n while {1} {\n  if {[gets stdin line]} {\n   puts $fp $line\n   } else {\n   close $fp\n   break\n  }\n }\n}");
	fclose(fp);
	
	fp = fopen("test.tcl","w+");
    if(fp==NULL)
        while(1);
	fprintf(fp,"for {set i 0} {$i < 10} {incr i} {\n puts $i\n led blue on\n delay 500\n led blue off\n delay 500 \n}");
	fclose(fp);
}

int tcl_async_handler(ClientData clientData,Tcl_Interp *interp, int code)
{
	return TCL_BREAK;
}
Tcl_AsyncHandler tcl_isr_hndl;
int main(void)
{
	systick_init(1000);
	virtual_efs_init();
	virtual_tcl_fs_init();
	gpio_portf_enable();
	
	tcl_isr_hndl = Tcl_AsyncCreate(tcl_async_handler,NULL);
	
    task_led_init();
	gpio_portf_intr_enable(SW1_BIT, 0, 0, 0);
	nvic_set_priority(&NVIC_PRI7_R, 2, 5);
    nvic_enable_irq(30);
	
	tcl_main();
	
	return 0;
}


void systick_handler(void)
{
	task_led_systickintr_handler();
}

void portf_handler(void)
{
	if (check(GPIO_PORTF_RIS_R, SW1_BIT))
    {
        //clear interrupt source
        sbit(GPIO_PORTF_ICR_R, SW1_BIT);

        Tcl_AsyncMark(tcl_isr_hndl);

        //clear interrupt source
        sbit(GPIO_PORTF_ICR_R, SW1_BIT);
        
    }
	
}
