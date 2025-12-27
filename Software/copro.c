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

#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "Debug.h"
#include "EPD_2in13_V4.h"
#include <stdlib.h> // malloc() free()
#include "img_check.h"
#include "img_bq.h"

uint32_t bqm3_min = 0;
uint32_t bqm3 = 0;
uint32_t bqm3_max = 0;
uint32_t bq = 0;
char str[20];


UBYTE BlackImage[EPD_2in13_V4_WIDTH / 8 + 1][EPD_2in13_V4_HEIGHT];

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
	
    sleep_ms(1000); 
    DEV_Module_Init();
	EPD_2in13_V4_Init();
    EPD_2in13_V4_Clear();
    //Create a new image cache
    
    UWORD Imagesize = ((EPD_2in13_V4_WIDTH % 8 == 0)? (EPD_2in13_V4_WIDTH / 8 ): (EPD_2in13_V4_WIDTH / 8 + 1)) * EPD_2in13_V4_HEIGHT;
    Paint_NewImage((uint8_t*)BlackImage, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);


    //Paint_Clear(WHITE);
    Paint_DrawBitMap(img_bq);
    bqm3 = 451;
    bqm3_min = 80;
    bqm3_max = 120;

    hours = 07;
    min = 02;

         if (bqm3 >= 10000) Paint_DrawNum( 0, 27, bqm3, &Font32,true, BLACK, WHITE);
    else if (bqm3 >= 1000 ) Paint_DrawNum(32, 27, bqm3, &Font32,true, BLACK, WHITE);
    else if (bqm3 >= 100  ) Paint_DrawNum(16, 0, bqm3, &Font48,true, BLACK, WHITE);
    else if (bqm3 >= 10   ) Paint_DrawNum(16+48, 0, bqm3, &Font48,true, BLACK, WHITE);
    else                  Paint_DrawNum(16+96, 0, bqm3, &Font48,true, BLACK, WHITE);

    int x1 = bqm3_min /2;
    int x2 = bqm3_max /2;

        if (x1 < 249)
    {
        if (x2 > 249)
        {
            x2 = 249;
        }
        for (int i=0; i< 7; i++)
        {
            Paint_DrawLine(x1, 100+i, x2, 100+i, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        }
    }
    x2 = hours * 2;
    if (min >= 30)
    {
        x2+=1;
    }
    if (x2 > 48)
    {
        x2 = 48;
    }
    sprintf(str,"%02d:%02d", hours, min);
    Paint_DrawString_EN(187, 0,str, &Font16, false, BLACK, WHITE);
    Paint_DrawLine(191     , 16,   191     , 16+4, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(192 + 48, 16,   192 + 48, 16+4, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(191     , 16,   192 + 48, 16, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(191     , 16+4, 192 + 48, 16+4, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    for (int i=0; i< 4; i++)
    {
        Paint_DrawLine(191     , 16+i, 192 + x2, 16+i, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    }


/*
    Paint_DrawBitMap(img_check);

    Paint_DrawString_EN(5,   0, "50Hz", &Font24, BLACK, WHITE);
    Paint_DrawString_EN(85,  0, "level", &Font24, BLACK, WHITE);
    Paint_DrawString_EN(178, 0, "rise", &Font24, BLACK, WHITE);

    Paint_DrawNum      (25,    95, 2, &Font16, BLACK, WHITE);
    Paint_DrawString_EN(25,   110,"mV", &Font12, BLACK, WHITE);
    Paint_DrawNum      (100,   95, 1402, &Font16, BLACK, WHITE);
    Paint_DrawString_EN(110,  110, "mV", &Font12, BLACK, WHITE);
    Paint_DrawNum      (185,  95, 12, &Font16, BLACK, WHITE);
    Paint_DrawString_EN(185, 110, "mV/10s", &Font12, BLACK, WHITE);
*/
    EPD_2in13_V4_Display_Base((uint8_t*)BlackImage);

    DEV_Delay_ms(5000);


    EPD_2in13_V4_Sleep();
    //free(BlackImage);
    //BlackImage = NULL;
    DEV_Delay_ms(2000);//important, at least 2s
    // close 5V
    Debug("close 5V, Module enters 0 power consumption ...\r\n");
    DEV_Module_Exit();



    while(1)
    {








         //TM1637_clear();
        //while(1);
        if 
        (
            (bqm3 > 9999) ||
            (bq >= (9999/INV_VOLUME))
        )
        {
            //TM1637_display(bq, false);
            //sleep_ms(1000);
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

            /*
            if (hours > 99)
            {
                TM1637_display_word("days", true);      sleep_ms(TIME_MS_TEXT); 
                TM1637_display(days, false);            sleep_ms(TIME_MS_VALUE);  
            }
            else if (hour > 0)
            {
                TM1637_display_word("tIME", true);      sleep_ms(TIME_MS_TEXT); 
                TM1637_display_both(hours, min, true);  sleep_ms(TIME_MS_VALUE);  
            }
            else
            {
                TM1637_display_word("tIME", true);      sleep_ms(TIME_MS_TEXT);
                for (int i=0; i< (TIME_MS_VALUE/200); i++)
                {
                    sec  = (time_s / 1) % 60;
                    min  = (time_s / 60) % 60;  
                    TM1637_display_both(min, sec, true);    sleep_ms(200);
                }  
            }

            TM1637_display_word("cnt", true);      sleep_ms(TIME_MS_TEXT);
            for (int i=0; i< (TIME_MS_VALUE/400); i++)
            {
                TM1637_display(counts % 10000, false); sleep_ms(200);  
                if (counts >= 10000)
                {
                    TM1637_clear(); 
                }
                sleep_ms(200);
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
            
            TM1637_clear(); sleep_ms(5000);
            */

            
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
    //printf("copro\n");
}