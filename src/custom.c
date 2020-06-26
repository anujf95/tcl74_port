#include "custom.h"
#include "tcl.h"
#include "delay.h"
#include "gpio.h"

int delayCmd(ClientData clientData, Tcl_Interp *interp, int argc,  char *argv[])
{
	unsigned int delay;
	if(argc < 2 )
	{
		Tcl_AppendResult(interp, "Invalid delay value", (char *) NULL);
		return TCL_ERROR;
	}
	sscanf(argv[1],"%u",&delay);
	
	delay_ms(delay);
	
	return TCL_OK;
}

/*************************LED Control****************************/
typedef struct
{
    uint16_t cnt;
    uint16_t on_cnt_max;
    uint16_t off_cnt_max;
    int8_t state;
    volatile uint32_t *reg;
    uint32_t set_mask;
}led_ctl_t;

volatile uint32_t ticks = 0;
volatile uint32_t gint32 = 0;

led_ctl_t red_led, green_led, blue_led,all_led;
static void led_blink(led_ctl_t *led)
{
    if(led->state==0)    //off
    {
        if(led->cnt < led->off_cnt_max)
            led->cnt++;
        else
        {
            led->state = 1;
            led->cnt   = 0;
            *led->reg |= led->set_mask;
        }
    }
    else if(led->state==1)    //on
    {
        if(led->cnt < led->on_cnt_max)
            led->cnt++;
        else
        {
            led->state = 0;
            led->cnt   = 0;
            *led->reg &= ~(led->set_mask);
        }
    }

}
void task_led_systickintr_handler(void)
{
    led_blink(&red_led);
    led_blink(&green_led);
    led_blink(&blue_led);
    led_blink(&all_led);
}
void task_led_init(void)
{
    red_led.state = -1;     //disable blink
    red_led.reg = &GPIO_PORTF_DATA_R;
    red_led.set_mask = (1<<LED_R_BIT);

    green_led.state = -1;   //disable blink
    green_led.reg = &GPIO_PORTF_DATA_R;
    green_led.set_mask = (1<<LED_G_BIT);

    blue_led.state = -1;    //disable blink
    blue_led.reg = &GPIO_PORTF_DATA_R;
    blue_led.set_mask = (1<<LED_B_BIT);

    all_led.state = -1;    //disable blink
    all_led.reg = &GPIO_PORTF_DATA_R;
    all_led.set_mask = (1<<LED_R_BIT) | (1<<LED_G_BIT) | (1<<LED_B_BIT);

}

int LedCmd(ClientData clientData, Tcl_Interp *interp, int argc,  char *argv[])
{
    led_ctl_t *led;
    if (!strcmp(argv[1], "red"))
        led = &red_led;
    else if (!strcmp(argv[1], "green"))
        led = &green_led;
    else if (!strcmp(argv[1], "blue"))
        led = &blue_led;
    else if (!strcmp(argv[1], "all"))
        led = &all_led;
    else
	{
		Tcl_AppendResult(interp, "Invalid LED color \"",
		argv[1],"\"", (char *) NULL);
		return TCL_ERROR;
	}

    /*******************************************************/
    if (!strcmp(argv[2], "on"))
    {
        *led->reg |=  led->set_mask;
        led->state = -1;
    }
    else if (!strcmp(argv[2], "off"))
    {
        *led->reg &= ~(led->set_mask);
        led->state = -1;
    }
    else if (!strcmp(argv[2], "blink"))
    {

        if (argc>=4)
        {
            if (sscanf(argv[3],"%u",(unsigned int *) &led->on_cnt_max) < 1)
            {
				Tcl_AppendResult(interp, "Invalid ON Time \"",
				argv[3],"\"", (char *) NULL);
				return TCL_ERROR;
			}

            if(argc>=5)
            {
                if(sscanf(argv[4],"%u",(unsigned int *)&led->off_cnt_max)<1)
                {
					Tcl_AppendResult(interp, "Invalid OFF Time \"",
					argv[4],"\"", (char *) NULL);
					return TCL_ERROR;
				}
            }
            else
                led->off_cnt_max = led->on_cnt_max;
        }
        else
        {
            led->on_cnt_max = 500;  //default LED on  time
            led->off_cnt_max = 500; //default LED off time
        }

        led->cnt = 0;
        led->state = 1;    //off initally
        if(led->state==1)
            *led->reg |= led->set_mask;
        if(led->state==0)
            *led->reg &= ~(led->set_mask);

    }
    else if (!strcmp(argv[2], "toggle"))
    {
        *led->reg ^= led->set_mask;
        led->state = -1;
    }
    else
    {  
		Tcl_AppendResult(interp, "Invalid Command\"",
		argv[2],"\"", (char *) NULL);
		return TCL_ERROR;
    }

    return TCL_OK;
}


