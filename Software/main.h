/**
 sebulli.com
 */

#ifndef MAIN_H
#define MAIN_H

extern volatile uint32_t counts;
extern volatile uint32_t time_s;
extern volatile uint32_t time_s_total;
extern volatile uint32_t time_meas_s;
extern volatile uint32_t time_meas_latch_s;
extern volatile bool preliminary;
extern volatile bool update;
extern volatile float goertzel_result;
extern volatile bool goertzel_calc;
extern volatile uint32_t adc_level;
extern volatile uint32_t adc_inc;
extern volatile bool charge_discharge_cmd;
extern volatile bool detection_active;
extern volatile uint32_t adc_bat_volt;

void Main_Debug(void);
bool Main_Task (struct repeating_timer *t);
void Gortzel_Result(void);
void Gortzel_Start(void);


#endif