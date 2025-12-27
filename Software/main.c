/**
 sebulli.com
 */
#include "tusb.h"
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
#include "hardware/pwm.h"

#include "global.h"
#include "copro.h"
#include "main.h"

#define DISCHARGE_TIME 50

uint32_t time_x = 0;
uint32_t time_100us = 0;
uint32_t time_s = 0;
uint32_t time_s_old = 0;
uint16_t adc_result = 0;
uint32_t adc_result_filt = 0;
uint32_t adc_result_filtL = 0;
uint32_t counts = 0;
uint8_t scope_val_L = 0;
uint8_t scope_val_H = 0;
bool scope_active = false;

volatile bool goertzel_active = false;
volatile bool goertzel_calc = false;
bool goertzel_display = false;
uint32_t discharge_cnt = 0;
bool is_charge = false;
bool charge_discharge_cmd = false;
uint32_t goertzel_n = 0;
uint16_t samples[10000];
uint slice_num;
void on_pwm_wrap();
bool output_adc = false;

// Goertzel parameter
const float SAMPLE_RATE = 10000.0;   // sample frequency
//float TARGET_FREQ = 50.0;     // target frequency
float fx = 0.0f;

const int N = 10000;                 // window length
float goertzel_result = 0.0f;
int k=0;
float omega = 0.0f;
float cw = 0.0f;
float c = 0.0f;
float z1 = 0.0;
float z2 = 0.0;
float s = 0.0;
    
void Gortzel_Calc(float frq);

int main()
{
    stdio_init_all();
  
    // Start the 2nd core
    multicore_launch_core1(Core1_Main);

    gpio_init(DISCHARGE);
    //gpio_set_dir(DISCHARGE, GPIO_IN);
    gpio_set_pulls(DISCHARGE, false, false);
    //adc_gpio_init(DISCHARGE);

    gpio_init(DISCHARGE);
    gpio_set_dir(DISCHARGE, GPIO_OUT);
    gpio_put(DISCHARGE, 0);


  

    // Initialize the ADC
    adc_init();
    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(SENSOR);
    // Select ADC input 3 (GPIO29)
    adc_select_input(3);
    

    // Tell the HV pin that the PWM is in charge of its value.
    gpio_set_function(HV_PWM, GPIO_FUNC_PWM);
    // Figure out which slice we just connected to the LED pin
    slice_num = pwm_gpio_to_slice_num(HV_PWM);

    
    // Mask our slice's IRQ output into the PWM block's single interrupt line,
    // and register our interrupt handler
    pwm_clear_irq(slice_num);
    pwm_set_irq_enabled(slice_num, true);
    irq_set_exclusive_handler(PWM_DEFAULT_IRQ_NUM(), on_pwm_wrap);
    irq_set_enabled(PWM_DEFAULT_IRQ_NUM(), true);
    
    // Get some sensible defaults for the slice configuration. By default, the
    // counter is allowed to wrap over its maximum range (0 to 2**16-1)
    pwm_config config = pwm_get_default_config();

    // 125MHz / 12.5 / 1000 = 10kHz
    config.top = 1000;
    // Set divider, reduces counter clock to sysclock/this value
    pwm_config_set_clkdiv(&config, 12.5f);
    // Load the configuration into our PWM slice, and set it running.
    pwm_init(slice_num, &config, true);

    pwm_set_gpio_level(HV_PWM, 0);//config.top / 2); // 50% PWM



 
    // Endless loop
    while (true)
    {

        if (time_s_old != time_s)
        {
            time_s_old = time_s;
            if (((time_s % 2)== 0) && (goertzel_calc == false))
            {
                goertzel_active = true; 
            }
        }


        if (goertzel_calc)
        {
            /*
            if (goertzel_display)
            {
                for (int i=0;i<10000;i++)
                {
                    samples[i] =2048 + 10.0f * sinf((2.0f*M_PI*(float)i*50.0f)/((float)10000)) / ADC_2_MV;
                }
                

                for (fx = 10.0f; fx <= 90.0f; fx += 1.0f ) //215
                {
                    Gortzel_Calc(fx);
                }
            }
            else
            {
                Gortzel_Calc(50.0f);
            }
            */
            Gortzel_Calc(50.0f);


        }

        if (tud_cdc_connected)
        {
            int32_t c = tud_cdc_read_char();
            if (c != -1)
            {
                switch (c)
                {
                    case 'A': 
                        scope_active = true ;
                        break;

                    case 'a':
                        scope_active = false;
                        break;

                    case 'N':
                        goertzel_display = true;
                        break;

                    case 'n':
                        goertzel_display = false;
                        break;

                    case 'D':
                        is_charge = true;
                        charge_discharge_cmd = true;
                        break;

                    case 'd':
                        is_charge = false;
                        charge_discharge_cmd = true;
                        break;

                    case 'o':
                        printf("ADC:%d\r\n",adc_result);
                        break;

                    case '?':
                    case 'h':
                    case 'H':
                        printf("\r\npion - the Raspberry Pi ionization chamber\r\n");
                        printf("Version:%s\r\n\r\n",PRG_VERSION);
                        printf("Commands:\r\n");
                        printf("?: This help text\r\n");
                        printf("A: Enable analog data output\r\n");
                        printf("a: Disable analog data output\r\n");
                        printf("N: Start measuring the 50Hz noise\r\n");
                        printf("n: Stop measuring the 50Hz noise\r\n");
                        printf("D: Charge\r\n");
                        printf("d: Discharge\r\n");
                        printf("o: Output ADC\r\n");
                        putchar('>');
                        break;
                }

            }
        } 
    }

    return 0;
}


void on_pwm_wrap()
{
        // Clear the interrupt flag that brought us here
    pwm_clear_irq(slice_num);
//pwm_set_gpio_level(PIN_UV_LED, fade * (50 + fade / 2)); // fade);
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



    if (adc_result > 2800 || charge_discharge_cmd)
    {
        charge_discharge_cmd = false;
        adc_result = 0;
        discharge_cnt = DISCHARGE_TIME;
    }
    
    if (discharge_cnt > 0)
    {
        discharge_cnt --;
        gpio_init(SENSOR);
        gpio_set_dir(SENSOR, GPIO_OUT);
        if (is_charge)
        {
            gpio_put(SENSOR, 1);
        }
        else
        {
            gpio_put(SENSOR, 0);
        }
        
    }
    else
    {
        gpio_set_dir(SENSOR, GPIO_IN);
        adc_gpio_init(SENSOR);
    }

    scope_val_L = (uint8_t)(((adc_result     ) & 0x003F));
    scope_val_H = (uint8_t)(((adc_result >> 6) & 0x003F));
    adc_result_filtL += (adc_result - adc_result_filt);
    adc_result_filt = adc_result_filtL / 128;

    

    if (goertzel_active)
    {
        samples[goertzel_n] = adc_result;
        goertzel_n ++;
        if (goertzel_n >= 10000)
        {
            goertzel_n = 0;
            goertzel_active = false;
            goertzel_calc = true;
        }
    }

    if (scope_active)
    {
        putchar(scope_val_L + 32 + 128);
        putchar(scope_val_H + 32);
    }
}

void Gortzel_Calc(float frq)
{
    //k = (int)(0.5f + ((N * f) / SAMPLE_RATE)); // Bin-Index
    float fk = (((float)N * frq) / (float)SAMPLE_RATE); // Bin-Index
    omega = (float)((2.0f * M_PI * fk) / N);
    cw = cosf(omega);
    c = 2.0f * cw;


    z1 = 0.0f;
    z2 = 0.0f;
    s = 0.0f;
     
    for (int i = 0; i < 10000; i++)
    {
        float win = (0.54f - 0.46f * cosf(2.0f * M_PI * (float)i / (float)(N - 1)));
        s = ((float)samples[i]) * win + c * z1 - z2;
        z2 = z1;
        z1 = s;
    }
    // Calculate the result
    goertzel_result = sqrtf(z2 * z2 + z1 * z1 - c*z1 * z2) * 2.0f * 0.000368f * ADC_2_MV;
    if (goertzel_display)
    {
        printf("fk:%f c:%f %fHz:%fmV\r\n",fk,c, frq, goertzel_result);
    }
    goertzel_calc = false;
}

void Main_Debug(void)
{
    //printf("time_s %d\n", time_s);
    //printf("adc %d\n", adc_result);
}