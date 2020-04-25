#include <sys/kserv.h> 
#include <sys/ipc.h> 
#include <sys/proc.h> 

#define KSERVD_PID 1

int kserv_reg(int kserv_id) {
	int res = -1;
	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);

	proto_add_int(&in, kserv_id);
	if(ipc_call(KSERVD_PID, KSERV_CMD_REG, &in, &out) == 0) {
		res = proto_read_int(&out);
	}

	proto_clear(&in);
	proto_clear(&out);
	return res;
}

int kserv_get(int kserv_id) {
	int res = -1;
	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);

	proto_add_int(&in, kserv_id);
	if(ipc_call(KSERVD_PID, KSERV_CMD_GET, &in, &out) == 0) {
		res = proto_read_int(&out);
	}

	proto_clear(&in);
	proto_clear(&out);
	return res;
}

int kserv_unreg(int kserv_id) {
	int res = -1;
	proto_t in, out;
	proto_init(&in, NULL, 0);
	proto_init(&out, NULL, 0);

	proto_add_int(&in, kserv_id);
	if(ipc_call(KSERVD_PID, KSERV_CMD_UNREG, &in, &out) == 0) {
		res = proto_read_int(&out);
	}

	proto_clear(&in);
	proto_clear(&out);
	return res;
}

static kserv_handle_t _kserv_handle;

static void handle_ipc(int pid, int cmd, void* p) {
	proto_t* in = ipc_get_arg();

	proto_t out;
	proto_init(&out, NULL, 0);

	_kserv_handle(pid, cmd, in, &out, p);

	proto_free(in);
	ipc_set_return(&out);
	proto_clear(&out);
	ipc_end();
}

void kserv_run(kserv_handle_t handle, void* p, bool prefork) {
	_kserv_handle = handle;

	proc_ready_ping();
	ipc_setup(handle_ipc, p, prefork);
}
