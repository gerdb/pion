/**
 sebulli.com
 */

#ifndef MAIN_H
#define MAIN_H

extern uint32_t counts;
extern uint32_t time_s;
extern uint32_t time_100us;

void Main_Debug(void);
bool Main_Task (struct repeating_timer *t);



#endif