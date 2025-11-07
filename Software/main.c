/**
 sebulli.com
 */



#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "pico/multicore.h"
#include "pico/stdlib.h"
#include <pico/time.h>

#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/structs/clocks.h"

#include "global.h"
#include "copro.h"
#include "main.h"


uint32_t time_x = 0;
uint32_t time_100us = 0;
uint32_t time_s = 0;
uint16_t adc_result = 0;
uint32_t adc_result_filt = 0;
uint32_t adc_result_filtL = 0;
uint32_t counts = 0;
uint8_t scope_val_L = 0;
uint8_t scope_val_H = 0;

int main()
{
    stdio_init_all();

    // Start the 2nd core
    multicore_launch_core1(Core1_Main);

    // Initialize the ADC
    adc_init();
    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(PIN_ADC);
    // Select ADC input 0 (GPIO26)
    adc_select_input(0);
    
    // Setup the task timer
    struct repeating_timer timer;

    // Create a 1ms task
    add_repeating_timer_us(-100, Main_Task, NULL, &timer); 

    // Endless loop
    while (true);

    return 0;
}



bool Main_Task (struct repeating_timer *t)
{
    time_x ++;
    time_100us ++;
    
    if (time_x >= 10000)
    {
        time_x = 0;
        time_s ++;
        //counts = time_s * 10 * time_s;
        counts ++;
    }
     
    adc_result = adc_read();
    scope_val_L = (uint8_t)(((adc_result     ) & 0x003F));
    scope_val_H = (uint8_t)(((adc_result >> 6) & 0x003F));
    adc_result_filtL += (adc_result - adc_result_filt);
    adc_result_filt = adc_result_filtL / 128;

    //putchar(100);
    putchar(scope_val_L + 32 + 128);
    putchar(scope_val_H + 32);

    return true;
}

void Main_Debug(void)
{
    //printf("time_s %d\n", time_s);
    //printf("adc %d\n", adc_result);
}