/**
 sebulli.com
 */

#ifndef MAIN_H
#define MAIN_H

extern uint32_t counts;
extern uint32_t time_s;
extern uint32_t time_100us;
//extern volatile bool goertzel_calc_X;
//extern volatile bool goertzel_active_X;
//extern uint32_t goertzel_n;
//extern uint16_t samples[];

void Main_Debug(void);
bool Main_Task (struct repeating_timer *t);



#endif