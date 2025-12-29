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
uint32_t time_x_6 = 0;
uint32_t time_100us = 0;
volatile uint32_t time_s = 0;
uint16_t adc_result = 0;
volatile uint32_t adc_level = 0;
volatile uint32_t adc_inc = 0;
uint32_t adc_old = 0;

uint32_t filt = 0;
uint32_t filt_cnt = 0;
volatile uint32_t filt_val = 0;
volatile bool filt_val_available = false;
volatile bool filt_reset = false;
uint32_t filt_val_old = 0;
int32_t ringcnt = 0;
int32_t ringcnt_d = 0;
int32_t ring_d2[8] = {0};
int32_t ring_d3[128] = {0};
int32_t histogram[101] = {0};
int32_t model2 = 0;
int32_t modelB_hp = 0;
bool pulse_det = false;
int32_t pulse_A = 0;
int32_t d_sum3 = 0;

uint32_t adc_result_filt = 0;
uint32_t adc_result_filtL = 0;
volatile bool update = false;
volatile uint32_t counts = 0;
volatile int32_t scopeval = 0;
uint8_t scope_val_L = 0;
uint8_t scope_val_H = 0;
bool scope_active = true;
uint32_t mux = 0;
uint32_t mux_val = 0;
volatile int32_t suppress_pulse_cnt = 0;

volatile bool goertzel_active = false;
volatile bool goertzel_calc = false;
bool goertzel_display = false;
uint32_t discharge_cnt = 0;
bool is_charge = false;
volatile bool charge_discharge_cmd = true;
uint32_t goertzel_n = 0;
uint slice_num;

bool output_adc = false;
bool piezo = false;
uint32_t piezo_cnt = 0;
volatile bool detection_active = false;


// Goertzel parameter
const float SAMPLE_RATE = 10000.0;   // sample frequency
//float TARGET_FREQ = 50.0;     // target frequency
float fx = 0.0f;

const int N = 10000;                 // window length
volatile float goertzel_result = 0.0f;
int k=0;
float omega = 0.0f;
float cw = 0.0f;
float c = 0.0f;
float z1 = 0.0;
float z2 = 0.0;
float s = 0.0;
    
void Gortzel_Prepare(float frq);
void on_pwm_wrap();


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

    gpio_init(PIEZO);
    gpio_set_dir(PIEZO, GPIO_OUT);
    gpio_put(PIEZO, 0);
  

    // Initialize the ADC
    adc_init();
    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(SENSOR);
    // Select ADC input 3 (GPIO29)
    adc_select_input(3);
    
    Gortzel_Prepare(50.0f);
    Gortzel_Start();

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
        if (filt_val_available)
        {
            filt_val_available = false;

            if (filt_reset || !detection_active)
            {
                filt_reset = false;
                d_sum3 = 0;
                model2 = 0;
                for (int32_t ii = 0; ii < 8; ii++)
                {
                    ring_d2[ii]= 0;
                }
                for (int32_t ii = 0; ii < 101; ii++)
                {
                    histogram[ii] = 0; 
                }

            }
            
            int32_t d2 = filt_val - filt_val_old;
            
            filt_val_old = filt_val;
            d_sum3 += d2 - ring_d2[ringcnt_d];
            ring_d2[ringcnt_d] = d2;

            ringcnt_d++;
            if (ringcnt_d >= 8)
            {
                ringcnt_d = 0;
            }

            int32_t vor = d_sum3 / 32 + 50;
            int32_t nachkomma = d_sum3 % 32;
            if (vor >0 && vor < 100)
            {
                histogram[vor] += 32 - nachkomma;
                histogram[vor+1] += nachkomma;
            }

            int32_t max = 0;
            int32_t maxi = 0;
            for (int32_t ii = 0; ii < 101; ii++)
            {
                if (histogram[ii] > max)
                {
                    max = histogram[ii];
                    maxi = ii;
                }
                histogram[ii] -= histogram[ii] / 128;
            }
            if (maxi > 2 && maxi < 98)
            {
                int32_t v1 = histogram[maxi - 1];
                int32_t v2 = histogram[maxi    ];
                int32_t v3 = histogram[maxi + 1];
                if (v2 > 0)
                {
                    int32_t mi_d = (maxi - 50)*32 + ((v3 - v1)*32) / v2;
                    model2 += mi_d;
                }
            }

            int32_t in_fil = (int32_t)filt_val*8 - model2;
            scopeval = in_fil /256;
            modelB_hp = (in_fil - ring_d3[ringcnt]) /256;
            

            if (modelB_hp>16 && !pulse_det)
            {
                pulse_A = ring_d3[ringcnt];
                pulse_det = true;
            }
            if (modelB_hp < 8 && pulse_det)
            {
                pulse_det = false;
                int pulse_hight = in_fil - pulse_A;

                // A valid pulse?
                if(
                    (pulse_hight > 5000) && 
                    (suppress_pulse_cnt == 0) &&
                    detection_active
                )
                {
                    piezo = true;
                    counts ++;
                }

            }
            
            ring_d3[ringcnt] = in_fil;
            ringcnt++;
            if (ringcnt>=25)
            {
                ringcnt = 0;
            }
        }

    }

    return 0;
}


void on_pwm_wrap()
{
    // Clear the interrupt flag that brought us here
    pwm_clear_irq(slice_num);

    time_x ++;
    time_100us ++;
    
    // 10kHz PWM = 100µs task
    if (time_x >= 10000)
    {
        time_x = 0;
        time_x_6 ++;
        if (time_x_6 >= 6)
        {
            time_x_6 = 0;
            adc_inc = (adc_level - adc_old) * 10;
            adc_old = adc_level;
        }

        if (suppress_pulse_cnt > 0)
        {
            suppress_pulse_cnt --;
            time_x_6 = 0;
            adc_old = adc_level;
        }
        else
        {
            time_s ++;
        }
    }
     
    adc_result = adc_read();
    adc_level = adc_result;

    filt += adc_result;
    filt_cnt++;
    if (filt_cnt >= 200)
    {
        filt_val = filt;
        filt_cnt = 0;
        filt = 0;
        filt_val_available = true;
    }

    // suppress the pulse detection around n x 512. 
    // See https://pip-assets.raspberrypi.com/categories/814-rp2040/documents/RP-008371-DS-1-rp2040-datasheet.pdf?disposition=inline 
    // chapter 4.9.4. INL and DNL
    int adc_lsb = adc_result & 0x01FF;
    if ( adc_lsb >= 0x01FF && adc_lsb <= 0x0001)
    {
        suppress_pulse_cnt = 10;
    }

    // Discharge request?
    // Cischarge at ADC 3600 = 2.9V
    if (adc_result > 3600 || charge_discharge_cmd)
    {
        charge_discharge_cmd = false;
        adc_result = 0;
        discharge_cnt = DISCHARGE_TIME;
        update = true;
        suppress_pulse_cnt = 10;
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
        filt_reset = true;
    }
    else
    {
        gpio_set_dir(SENSOR, GPIO_IN);
        adc_gpio_init(SENSOR);
    }


    adc_result_filtL += (adc_result - adc_result_filt);
    adc_result_filt = adc_result_filtL / 128;

    

    if (goertzel_active)
    {
        float win = (0.54f - 0.46f * cosf(2.0f * M_PI * (float)goertzel_n / (float)(N - 1)));
        s = ((float)adc_result) * win + c * z1 - z2;
        z2 = z1;
        z1 = s;
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
        scope_val_L = (uint8_t)(((adc_result     ) & 0x003F));
        scope_val_H = (uint8_t)(((adc_result >> 6) & 0x003F));
        putchar(scope_val_L + 32 + 128);
        putchar(scope_val_H + 32);

        scope_val_L = (uint8_t)(((scopeval     ) & 0x003F));
        scope_val_H = (uint8_t)(((scopeval >> 6) & 0x003F));
        putchar(scope_val_L + 32);
        putchar(scope_val_H + 32);

        if (mux == 0) mux_val = 42;
        if (mux == 1) mux_val = goertzel_result*10.f;
        mux_val |= mux << 10;
        scope_val_L = (uint8_t)(((mux_val     ) & 0x003F));
        scope_val_H = (uint8_t)(((mux_val >> 6) & 0x003F));
        putchar(scope_val_L + 32);
        putchar(scope_val_H + 32);


        mux++;
        if (mux > 1)
        {
            mux = 0;
        }
    }

    if (piezo)
    {
        piezo_cnt = 0;
        piezo = false;
    }

    if (piezo_cnt < 400)
    {
        piezo_cnt++;
        // 3kHz
        if (piezo_cnt % 3 == 0)
        {
            gpio_put(PIEZO, 1);
        }
        else
        {
            gpio_put(PIEZO, 0);
        }
    }
}

void Gortzel_Prepare(float frq)
{
    //k = (int)(0.5f + ((N * f) / SAMPLE_RATE)); // Bin-Index
    float fk = (((float)N * frq) / (float)SAMPLE_RATE); // Bin-Index
    omega = (float)((2.0f * M_PI * fk) / N);
    cw = cosf(omega);
    c = 2.0f * cw;
}

void Gortzel_Start(void)
{
    z1 = 0.0f;
    z2 = 0.0f;
    s = 0.0f;
    goertzel_n = 0;
    goertzel_calc = false;
    goertzel_active = true;
}

void Gortzel_Result(void)
{
    // Calculate the result
    goertzel_result = sqrtf(z2 * z2 + z1 * z1 - c*z1 * z2) * 2.0f * 0.000368f * ADC_2_MV ;
    if (goertzel_display)
    {
        printf("time_s %f\n", time_s);
        printf("G: %fmV\r\n",goertzel_result);
    }
    goertzel_calc = false;
}

void Main_Debug(void)
{
    //printf("time_s %d\n", time_s);
    //printf("adc %d\n", adc_result);
}