#include <kernel/irq.h>
#include <kernel/kernel.h>
#include <kernel/hw_info.h>
#include "../timer_arch.h"

static uint32_t irq_enable_flag = 0;

void irq_arch_init(void) {
	//routing_core0_irq();
    irq_enable_flag = 0;
	gic_init(MMIO_BASE + 0x1840000);
    for(int i = 0; i < 1022; i++){
        gic_irq_disable(0, i);
    }
}

inline uint32_t irq_get(void) {
	int ack = gic_get_irq();
    int irqno = ack & 0x3FF;

    if(irqno == 27){
        return IRQ_TIMER0;
    }else if(irqno == 0){
        return IRQ_IPI;
    }
    return 0;
}

inline void irq_enable(uint32_t irq) {
    if(irq & irq_enable_flag)
        return;

	if(irq == IRQ_TIMER0){
        gic_irq_enable(0, 27);
        irq_enable_flag |= irq;
    }
}

void irq_disable(uint32_t irq) {
	(void)irq;
}