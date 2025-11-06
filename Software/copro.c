/**
 sebulli.com
 */

#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"

#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/structs/clocks.h"

#include "global.h"
#include "copro.h"
#include "main.h"
#include "PicoTM1637.h"

uint32_t bqm3_min = 0;
uint32_t bqm3 = 0;
uint32_t bqm3_max = 0;
uint32_t bq = 0;

#define LENGTH     (100)  // length of ionisation chamber im mm
#define DIAMETER    (70)  // diameter of ionisation chamber im mm

#define INV_VOLUME (1273885350 / (LENGTH*DIAMETER*DIAMETER)) // 1m³/my_volume 

#define TIME_MS_SCROLL          (500)
#define TIME_MS_TEXT            (2000)
#define TIME_MS_VALUE           (5000)
#define TIME_MS_VALUE_SHORT     (2000)

typedef enum
{
    TXT_SBLI    = 0,
    TXT_CNT     = 1,
    COUNT       = 2,
    TXT_BQ      = 3,
    BQ          = 4
} Display;

void Core1_Main(void)
{
    uint32_t sec = 0;
    uint32_t min = 0;
    uint32_t hour = 0;
    uint32_t hours = 0;
    uint32_t days = 0;
    Display display = TXT_SBLI;
    // Initialize the LED display
    TM1637_init(LED_DISP_CLK, LED_DISP_DIN);  
    TM1637_clear(); 
    
    TM1637_set_brightness(3); // 0..7
    
    TM1637_display_word("    ", true);sleep_ms(500);
    TM1637_display_word("   S", true);sleep_ms(TIME_MS_SCROLL);
    TM1637_display_word("  SE", true);sleep_ms(TIME_MS_SCROLL);
    TM1637_display_word(" SEB", true);sleep_ms(TIME_MS_SCROLL);
    TM1637_display_word("SEBU", true);sleep_ms(TIME_MS_SCROLL);
    TM1637_display_word("EBUL", true);sleep_ms(TIME_MS_SCROLL);
    TM1637_display_word("BULL", true);sleep_ms(TIME_MS_SCROLL);
    TM1637_display_word("ULLi", true);sleep_ms(TIME_MS_SCROLL);
    TM1637_display_word("LLi ", true);sleep_ms(TIME_MS_SCROLL);
    TM1637_display_word("Li  ", true);sleep_ms(TIME_MS_SCROLL);
    TM1637_display_word("i   ", true);sleep_ms(TIME_MS_SCROLL);
    TM1637_display_word("    ", true);sleep_ms(1000);

    while(1)
    {
        Calc();

        if 
        (
            (bqm3 > 9999) ||
            (bq >= (9999/INV_VOLUME))
        )
        {
            TM1637_display(bq, false);
            sleep_ms(1000);
        }
        else
        {
            //Main_Debug();
            //Core1_Debug();

            sec  = (time_s / 1) % 60;
            min  = (time_s / 60) % 60;
            hour = (time_s / (60*60)) % 24;
            hours = (time_s / (60*60));
            days  = (time_s / (60*60*24));

            if (hours > 99)
            {
                TM1637_display_word("days", true);      sleep_ms(TIME_MS_TEXT); 
                TM1637_display(days, false);            sleep_ms(TIME_MS_VALUE);  
            }
            else if (hour > 0)
            {
                TM1637_display_word("tiME", true);      sleep_ms(TIME_MS_TEXT); 
                TM1637_display_both(hours, min, true);  sleep_ms(TIME_MS_VALUE);  
            }
            else
            {
                TM1637_display_word("tiME", true);      sleep_ms(TIME_MS_TEXT);
                for (int i=0; i< (TIME_MS_VALUE/200); i++)
                {
                    sec  = (time_s / 1) % 60;
                    min  = (time_s / 60) % 60;  
                    TM1637_display_both(min, sec, true);    sleep_ms(200);
                }  
            }

            TM1637_display_word("cnt", true);      sleep_ms(TIME_MS_TEXT);
            if (counts >= 10000)
            {
                for (int i=0; i< (TIME_MS_VALUE/200); i++)
                {
                    TM1637_display(counts % 10000, false);  sleep_ms(200);
                    TM1637_clear();                         sleep_ms(200);
                }  
            } 
            else
            {
                TM1637_display(counts, false); sleep_ms(TIME_MS_VALUE);
            }

            TM1637_display_word("bqM3", true);  sleep_ms(TIME_MS_TEXT);

            TM1637_display(bqm3_min, false);      sleep_ms(TIME_MS_VALUE_SHORT);
            for (int i=0; i<10; i++)
            {
                TM1637_display(bqm3_min + (i*(bqm3-bqm3_min))/10, false); sleep_ms(100);
            }
            TM1637_display(bqm3, false);          sleep_ms(TIME_MS_VALUE);
            for (int i=0; i<10; i++)
            {
                TM1637_display(bqm3 + (i*(bqm3_max-bqm3))/10, false); sleep_ms(100);
            }
            TM1637_display(bqm3_max, false);      sleep_ms(TIME_MS_VALUE_SHORT); 

            TM1637_clear(); sleep_ms(1000);
        }
    }
}

void Calc(void)
{
    volatile uint32_t buf_counts;
    volatile uint32_t buf_time_s;
    volatile uint32_t buf_time_100us;
    static uint32_t last_counts=0;
    static uint32_t last_time=0;

    // Buffer value from other core
    buf_counts = counts;
    buf_time_s = time_s;
    buf_time_100us = time_100us;

    if (buf_time_s > 0)
    {
        // Difference since last measurement
        uint32_t d_counts = (buf_counts - last_counts);
        uint32_t d_time   = (buf_time_100us - last_time);

        // becquerel, counts = second (since last calculation)
        if (d_time > 0)
        {
            bq = (10000 * d_counts + 5000) / d_time;
        }

        // becquerel per m³ (since start of system)
        bqm3 = (INV_VOLUME * buf_counts) / buf_time_s;

        // Calculate inaccurace
        uint32_t inaccuracy = (INV_VOLUME * sqrt(buf_counts)) / buf_time_s;

        // 2 sigma = 95% of all values
        inaccuracy *= 2;
        if (inaccuracy <= bqm3)
        {
            bqm3_min = bqm3 - inaccuracy;
        }
        else
        {
            bqm3_min = 0;
        }

        bqm3_max = bqm3 + inaccuracy;

        
    }

    last_time = buf_time_100us;
    last_counts = buf_counts;

}

void Core1_Debug(void)
{
    printf("copro\n");
}