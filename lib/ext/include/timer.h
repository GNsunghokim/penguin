#ifndef __TIMER_H__
#define __TIMER_H__

#include <stdint.h>

extern uint64_t TIMER_FREQUENCY_PER_SEC;
extern uint64_t __timer_ms;
extern uint64_t __timer_us;
extern uint64_t __timer_ns;

void timer_init();
uint64_t timer_frequency();

uint64_t timer_s();
uint64_t timer_ms();
uint64_t timer_us();
uint64_t timer_ns();

void timer_swait(uint32_t s);	
void timer_mwait(uint32_t ms);
void timer_uwait(uint32_t us);
void timer_nwait(uint32_t ns);

#endif /* __TIMER_H__ */
