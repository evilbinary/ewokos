#include <ewoksys/core.h>
#include <ewoksys/syscall.h>
#include <ewoksys/proc.h>
#include <ewoksys/ipc.h>

#ifdef __cplusplus
extern "C" {
#endif

inline void schd_core_lock() {
	syscall0(SYS_SCHD_CORE_LOCK);
}

inline void schd_core_unlock() {
	syscall0(SYS_SCHD_CORE_UNLOCK);
}

int core_get_ux(void) {
	proto_t out;
	PF->init(&out);
	int res = ipc_call(get_cored_pid(), CORE_CMD_GET_UX, NULL, &out);
	if(res == 0)
		res = proto_read_int(&out);
	PF->clear(&out);
	return res;
}

int core_set_ux(int ux_index) {
	proto_t in;
	PF->init(&in)->addi(&in, ux_index);
	int res = ipc_call_wait(get_cored_pid(), CORE_CMD_SET_UX, &in);
	PF->clear(&in);
	return res;
}

#ifdef __cplusplus
}
#endif

