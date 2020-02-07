#ifndef CONTEXT_H
#define CONTEXT_H

#include <_types.h>

typedef struct {
  uint32_t cpsr, pc, gpr[ 13 ], sp, lr;
} context_t;

#endif
