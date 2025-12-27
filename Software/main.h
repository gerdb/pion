/**
 sebulli.com
 */

#ifndef MAIN_H
#define MAIN_H

extern volatile uint32_t counts;
extern volatile uint32_t time_s;
extern volatile bool update;
extern volatile float goertzel_result;
extern volatile uint32_t adc_level;
extern volatile uint32_t mv_inc;
extern volatile bool charge_discharge_cmd;

void Main_Debug(void);
bool Main_Task (struct repeating_timer *t);



#endif