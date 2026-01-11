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
#include "EPD_2in13_V4.h"

#include "img_intro.h"
#include "img_check.h"
#include "img_bq.h"
#include "img_bq_low_bat.h"

uint32_t bqm3_min = 0;
uint32_t bqm3 = 0;
uint32_t bqm3_max = 0;
char str[20];

uint32_t min = 0;
uint32_t hours = 0;

UBYTE BlackImage[EPD_2in13_V4_WIDTH / 8 + 1][EPD_2in13_V4_HEIGHT];

#define LENGTH     (100)  // length of ionisation chamber im mm
#define DIAMETER    (70)  // diameter of ionisation chamber im mm

#define INV_VOLUME (1273239544 / (LENGTH*DIAMETER*DIAMETER)) // 1m³/my_volume, 1273239544 = 4/pi * 1000³ 

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


    bool all_ok;


    // Initialize the LED display
	
    sleep_ms(1000); 
    UWORD Imagesize = ((EPD_2in13_V4_WIDTH % 8 == 0)? (EPD_2in13_V4_WIDTH / 8 ): (EPD_2in13_V4_WIDTH / 8 + 1)) * EPD_2in13_V4_HEIGHT;
    Paint_NewImage((uint8_t*)BlackImage, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
    
    DEV_Module_Init();
    EPD_2in13_V4_Init();
    Paint_DrawBitMap(img_intro);
    EPD_2in13_V4_Display_Base((uint8_t*)BlackImage);
    EPD_2in13_V4_Sleep();
    DEV_Delay_ms(2000);//important, at least 2s
    DEV_Module_Exit();

    sleep_ms(10000);
    Gortzel_Start();
    sleep_ms(5000); 
    do
    {
        all_ok = true;
        DEV_Module_Init();
        EPD_2in13_V4_Init();
        Paint_DrawBitMap(img_check);
        Paint_DrawString_EN(5,   0, "50Hz",  &Font24, false, BLACK, WHITE);
        Paint_DrawString_EN(85,  0, "level", &Font24, false, BLACK, WHITE);
        Paint_DrawString_EN(178, 0, "rise",  &Font24, false, BLACK, WHITE);
        
        // Calculate the result of the Goertzel filter?
        if (goertzel_calc)
        {
            Gortzel_Result();
        }

        // float result of goertzel_result to string
        sprintf(str,"%4.1f", goertzel_result);
        // already a result available?
        if (goertzel_result == 0.0f)
        {
            Paint_DrawString_EN(14,   92, "wait",  &Font16, false, WHITE, BLACK);
            all_ok = false;
        }
        // smaller than 2.0mV? Then result is good
        else if (goertzel_result < 2.0f)
        {
            Paint_DrawString_EN(14,   92, str,  &Font16, false, BLACK, WHITE);
        }
        // 50Hz noise is to high
        else
        {
            Paint_DrawString_EN(14,   92, str,  &Font16, false, WHITE, BLACK);
            all_ok = false;
        }

        Paint_DrawString_EN(36,   110,"mV", &Font12,false, BLACK, WHITE);

        float volt_level = (float)adc_level * ADC_2_V;

        // voltage to string
        sprintf(str,"%0.3f", volt_level);
        // valid level?
        if (volt_level > 0.5f && volt_level < 3.0f)
        {
            Paint_DrawString_EN(99,   92, str,  &Font16, false, BLACK, WHITE);
        }
        // level too low (or too high)
        else
        {
            Paint_DrawString_EN(99,   92, str,  &Font16, false, WHITE, BLACK);
            all_ok = false;
            charge_discharge_cmd = true;
        }
        Paint_DrawString_EN(121,  110, "V", &Font12,false, BLACK, WHITE);


        int mv_per_min = (int)(adc_inc * ADC_2_MV);
        // millivolts per minute to string
        sprintf(str,"%3d", mv_per_min);
        // valid increment?
        if (mv_per_min > 6.0f && mv_per_min < 600.0f)
        {
            Paint_DrawString_EN(195,   92, str,  &Font16, false, BLACK, WHITE);
        }
        // increment too low (or too high)
        else
        {
            Paint_DrawString_EN(195,   92, str,  &Font16, false, WHITE, BLACK);
            all_ok = false;
        }
        Paint_DrawString_EN(190, 110, "mV/min", &Font12,false, BLACK, WHITE);

        // Start new measurement
        if (!all_ok)
        {
            Gortzel_Start();
        }

        EPD_2in13_V4_Display_Base((uint8_t*)BlackImage);
        EPD_2in13_V4_Sleep();
        DEV_Delay_ms(2000);//important, at least 2s
        DEV_Module_Exit();

        // Wait 2+8 = 10s for next check
        DEV_Delay_ms(8000);
    } while (!all_ok);
    

    // Activate the radon detection
    detection_active = true;
    while(1)
    {

        if (update)
        {
            Calc();
        }

        float bat_volt_level = (float)adc_bat_volt * ADC_2_V * 7.66666f; // 7.6666 = 10k/1k5
        bool low_bat = bat_volt_level < 5.500f;

        DEV_Module_Init();
        EPD_2in13_V4_Init();

        if (low_bat)
        {
            Paint_DrawBitMap(img_bq_low_bat);
        }
        else
        {
            Paint_DrawBitMap(img_bq);
            if (preliminary)
            {
                for (int y=25; y< 90; y+=2)
                {
                    Paint_DrawLine(180, y, 249, y, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
                }
            }
        }
        

             if (bqm3 >= 10000) Paint_DrawNum( 0, 27, bqm3, &Font32,true, BLACK, WHITE);
        else if (bqm3 >= 1000 ) Paint_DrawNum(32, 27, bqm3, &Font32,true, BLACK, WHITE);
        else if (bqm3 >= 100  ) Paint_DrawNum(16, 0, bqm3, &Font48,true, BLACK, WHITE);
        else if (bqm3 >= 10   ) Paint_DrawNum(16+48, 0, bqm3, &Font48,true, BLACK, WHITE);
        else if (bqm3 >= 1    ) Paint_DrawNum(16+96, 0, bqm3, &Font48,true, BLACK, WHITE);
        else                    Paint_DrawString_EN(16, 27, "wait", &Font32,true, BLACK, WHITE);

        int ramp_per_day = 0;
        if (time_meas_latch_s != 0 && bqm3 != 0)
        {
            ramp_per_day = 86400 / time_meas_latch_s;   
        }

        Paint_DrawNum(187, 94,ramp_per_day, &Font16, false, BLACK, WHITE);

        int x1 = bqm3_min /2;
        int x2 = bqm3_max /2;

        if (x1 < 249 && x2 > 0 && x2 >= x1)
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

        if (low_bat)
        {
            // bat voltage to string
            sprintf(str,"%0.2fV", bat_volt_level);
            Paint_DrawString_EN(187,  75, str,  &Font16, false, WHITE, BLACK);
        }


        EPD_2in13_V4_Display_Base((uint8_t*)BlackImage);

        DEV_Delay_ms(1000);
        EPD_2in13_V4_Sleep();
        DEV_Delay_ms(2000);//important, at least 2s
        DEV_Module_Exit();

        update = false;
        while (!update);
    }
}



void Calc(void)
{
    uint32_t buf_counts;
    uint32_t buf_time_s;
    uint32_t buf_time_s_total;

    // Buffer value from other core
    buf_counts = counts;
    buf_time_s = time_s;
    buf_time_s_total = time_s_total;

    hours = buf_time_s_total / 3600;
    min = (buf_time_s_total / 60) % 60;

    if (buf_time_s >= 60)
    {
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
}

void Core1_Debug(void)
{
    //printf("copro\n");
}