#include <interrupt.h>
#include <kernel/interrupt.h>
#include <kernel/context.h>
#include <kernel/proc.h>
#include <stddef.h>
#include <kstring.h>
#include <kernel/schedule.h>
#include <kernel/irq.h>
#include <kernel/system.h>
#include <dev/ipi.h>
#include <mm/mmu.h>
#include <mm/kalloc.h>

typedef struct {
	int32_t  pid;
	uint32_t entry;
} interrupt_t;

static interrupt_t _interrupts[SYS_INT_MAX];

void interrupt_init(void) {
	memset(&_interrupts, 0, sizeof(interrupt_t)*SYS_INT_MAX);	
}

void interrupt_setup(proc_t* cproc, uint32_t interrupt, uint32_t entry) {
	_interrupts[interrupt].entry = entry;
	_interrupts[interrupt].pid = cproc->info.pid;

	if(cproc->space->inter_stack == 0) {
		uint32_t page = (uint32_t)kalloc4k();
		map_page(cproc->space->vm, page, V2P(page), AP_RW_RW, 0);
		cproc->space->inter_stack = page;
		flush_tlb();
	}
}

void  interrupt_send(context_t* ctx, uint32_t interrupt) {
	if(_interrupts[interrupt].pid <= 0 || _interrupts[interrupt].entry == 0)
		return;

	proc_t* proc = proc_get(_interrupts[interrupt].pid);
	proc_t* cproc = get_current_proc();
	if(proc == NULL)
		return;
	
	proc->space->interrupt.saved_state = proc->info.state;
	proc->space->interrupt.saved_block_by = proc->info.block_by;
	proc->space->interrupt.saved_block_event = proc->block_event;
	proc->space->interrupt.interrupt = interrupt;
	proc->space->interrupt.entry = _interrupts[interrupt].entry;
	irq_disable_cpsr(&proc->ctx.cpsr); //disable interrupt on proc

	if(proc->info.core == cproc->info.core) {
		proc->info.state = RUNNING;
		proc_switch(ctx, proc, true);
	}
	else {
		proc_ready(proc);
		schedule(ctx);
#ifdef KERNEL_SMP
		ipi_send(proc->info.core);
#endif
	}
}

void interrupt_end(context_t* ctx) {
	proc_t* cproc = get_current_proc();
	cproc->info.state = cproc->space->interrupt.saved_state;
	cproc->info.block_by = cproc->space->interrupt.saved_block_by;
	cproc->block_event = cproc->space->interrupt.saved_block_event;
	memcpy(ctx, &cproc->space->interrupt.ctx, sizeof(context_t));
	irq_enable_cpsr(&cproc->ctx.cpsr); //enable interrupt on proc
	schedule(ctx);
}

