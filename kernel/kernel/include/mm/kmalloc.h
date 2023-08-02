#ifndef KMALLOC_H
#define KMALLOC_H

/*km_alloc/km_free manage the kernel-reserved memory trunk(from KMALLOC_BASE, with size KMALLOC_SIZE), the memory trunk only reserved for kernel, not managed by MMU! */

#include <stdint.h>

void* kmalloc(uint32_t size);
void* kcalloc(uint32_t nmemb, uint32_t size);
void  kfree(void* p);
void  kmalloc_init(void);

#endif