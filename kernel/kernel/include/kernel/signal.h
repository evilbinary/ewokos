#ifndef PROC_SIGNAL_H
#define PROC_SIGNAL_H

#include <kernel/context.h>

struct st_proc;
typedef struct {
    uint32_t      entry;
    uint32_t      sig_no;

    bool          do_switch;
	uint32_t      stack; //mapped stack page
	saved_state_t saved_state;
} signal_t;

extern int32_t  proc_signal_setup(uint32_t entry);
extern void     proc_signal_send(context_t* ctx, struct st_proc* proc, int32_t sig_no, bool quick);
extern void     proc_signal_end(context_t* ctx);

#endif
