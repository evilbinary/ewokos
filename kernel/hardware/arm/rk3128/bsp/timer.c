#include <mm/mmu.h>
#include <dev/timer.h>
#include <kernel/irq.h>
#include <kernel/kernel.h>
#include <kprintf.h>
#include "timer_arch.h"

#define GIC_DEFAULT_FREQ	6000000
uint32_t _timer_tval  = 0;
uint32_t _cntfrq = GIC_DEFAULT_FREQ;

void __write_cntv_tval(uint32_t);
void __enable_cntv(void);

static inline uint32_t read_cntfrq(void) {
  uint32_t val;
  __asm__ volatile ("mrc p15, 0, %0, c14, c0, 0" : "=r"(val) );
  return val;
}

static inline uint32_t read_cntpct(void) {
  uint32_t val;
  __asm__ volatile ("mrc p15, 0, %0, c14, c0, 0" : "=r"(val) );
  return val;
}

inline void write_cntv_tval(uint32_t tval) {
	__asm__ volatile ("mcr p15, 0, %0, c14, c3, 0" :: "r"(tval));
}

static inline uint32_t read_cntv_tval(void) {
	uint32_t val;
	__asm__ volatile ("mrc p15, 0, %0, c14, c3, 0" :: "r"(val));
	return val;
}


static inline void enable_cntv(void) {
	__asm__ volatile ("mcr p15, 0, %0, c14, c3, 1" :: "r"(1));
}

static inline uint32_t disable_cntv(void) {
	__asm__ volatile("mcr p15, 0, %0, c14, C3, 1" :: "r" (0));
}

static inline uint64_t  read_cntvct(void) {
	uint64_t val;
	__asm__ volatile("mrrc p15, 1, %Q0, %R0, c14" : "=r" (val));
	return val;
}

static inline uint32_t read_cntctl(void) {
	uint32_t val;
	__asm__ volatile("mrc p15, 0, %0, c14, C3, 1" : "=r" (val));
	return val;
}

void timer_set_interval(uint32_t id, uint32_t times_per_sec) {
	(void)id;
	if (times_per_sec < MIN_TIMER_FREQ)
		times_per_sec = MIN_TIMER_FREQ;
	_cntfrq = read_cntfrq(); 
	if(_cntfrq < 1000000 || _cntfrq > 50000000)
		_cntfrq = GIC_DEFAULT_FREQ;
	_timer_tval = _cntfrq/times_per_sec;
	write_cntv_tval(_timer_tval);
	enable_cntv();
}

inline void timer_clear_interrupt(uint32_t id) {
	id = _timer_tval;
	write_cntv_tval(id);
}

/*do fast 64bit div constant
*	because 52 bit timer can provide 23 years cycle loop
* 	we can do 64bit divided as below：
* 	x / 24 = x / (4096/171) = x * 171 / 4096 = (x * 682) >> 12
*   it will save hundreds of cpu cycle
*/
static __inline uint64_t fast_div64_24(uint64_t x){
	return (x*171)>>12;
}

inline uint64_t timer_read_sys_usec(void) { //read microsec
	return fast_div64_24(read_cntvct());
}
