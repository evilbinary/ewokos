#include <mm/mmu.h>
#include <kernel/hw_info.h>
#include <dev/timer.h>
#include <kernel/irq.h>
#include <kernel/kernel.h>
#include <sbi.h>
#include <csr.h>

/*
The ARM Versatile 926EJS board contains two ARM SB804 dual timer modules [ARM Timers 2004]. Each timer module contains two timers, which are driven by the same clock. The base addresses of the timers are:
  Timer0: 0x101E2000, Timer1: 0x101E2020
  Timer2: 0x101E3000, Timer3: 0x101E3020
*/
#define DEFAULT_TIM_FREQ 23000000 

static uint32_t timer_interval;

void timer_set_interval(uint32_t id, uint32_t times_per_sec) {
  csr_set(sie, SIE_STIE);

  if (times_per_sec < MIN_TIMER_FREQ)
    times_per_sec = MIN_TIMER_FREQ;
  timer_interval = DEFAULT_TIM_FREQ / times_per_sec;

  sbi_set_timer(csr_read(CSR_TIME) + timer_interval);
}

void timer_clear_interrupt(uint32_t id) {
    //csr_clear(sie, SIE_STIE);
    sbi_set_timer(csr_read(CSR_TIME) + timer_interval);  
}

uint64_t timer_read_sys_usec(void) { //read microsec
	return csr_read(CSR_TIME)/10;
}
