#ifndef KERNEL_H
#define KERNEL_H

#include <mm/mmu.h>
#include <sramdisk.h>

extern char _kernel_start[];
extern char _kernel_end[];
extern char _init_stack_top[];
extern char _irq_stack_top[];

extern uint32_t _phy_mem_size;

extern page_dir_entry_t* _kernel_vm;
void set_kernel_vm(page_dir_entry_t* vm);
void set_allocable_vm(page_dir_entry_t* vm);


#endif
