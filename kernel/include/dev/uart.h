#ifndef UART_H
#define UART_H

#include <_types.h>

int32_t uart_dev_init(void);
int32_t uart_write(const void* data, uint32_t size);

#endif
