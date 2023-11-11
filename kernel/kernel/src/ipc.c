#include <kernel/proc.h>
#include <kernel/schedule.h>
#include <kernel/system.h>
#include <mm/kalloc.h>
#include <mm/kmalloc.h>
#include <mm/mmu.h>
#include <kernel/kernel.h>
#include <stddef.h>
#include <kstring.h>

#define IPC_BUFFER_SIZE 128

int32_t proc_ipc_setup(context_t* ctx, uint32_t entry, uint32_t extra_data, uint32_t flags) {
	(void)ctx;
	proc_t* cproc = get_current_proc();
	cproc->space->ipc_server.entry = entry;
	cproc->space->ipc_server.extra_data = extra_data;
	cproc->space->ipc_server.flags = flags;
	
	cproc->space->ipc_server.stack = proc_stack_alloc(cproc);
	return 0;
}

uint32_t proc_ipc_fetch(struct st_proc* serv_proc) {
	ipc_task_t* ipc = NULL;
	while(true) {
		ipc = (ipc_task_t*)queue_pop(&serv_proc->space->ipc_server.tasks);
		if(ipc == NULL) 
			return 0;
		proc_t* client_proc = proc_get(ipc->client_pid);
		if(client_proc != NULL && ipc->client_uuid == client_proc->info.uuid) //available ipc
			break;
		kfree(ipc); //drop unvailable ipc
	}

	serv_proc->space->ipc_server.ctask = ipc;
	return ipc->uid;
}
 
inline ipc_task_t* proc_ipc_get_task(struct st_proc* serv_proc) {
	return serv_proc->space->ipc_server.ctask;
}

int32_t proc_ipc_do_task(context_t* ctx, proc_t* serv_proc, uint32_t core) {
	ipc_task_t* ipc = proc_ipc_get_task(serv_proc);
	if(ipc == NULL ||
			ipc->state == IPC_IDLE ||
			ipc->uid == 0) {
		return -1;
	}

	proc_save_state(serv_proc, &serv_proc->space->ipc_server.saved_state);
	serv_proc->space->ipc_server.do_switch = true;

	if((ipc->call_id & IPC_LAZY) == 0)
		proc_switch_multi_core(ctx, serv_proc, core);
	return 0;
}

static void ipc_free(ipc_task_t* ipc) {
	proto_clear(&ipc->data);
	kfree(ipc);
}

ipc_task_t* proc_ipc_req(proc_t* serv_proc, proc_t* client_proc, int32_t call_id, proto_t* data) {
	if(queue_num(&serv_proc->space->ipc_server.tasks) >= IPC_BUFFER_SIZE) {
		printf("ipc request buffer overflowed! c: %d, s: %d\n", client_proc->info.pid, serv_proc->info.pid);
		//queue_clear(&serv_proc->space->ipc_server.tasks, ipc_free);
		return NULL;
	}

	_ipc_uid++;
	ipc_task_t* ipc  = (ipc_task_t*)kmalloc(sizeof(ipc_task_t));
	if(ipc == NULL)
		return NULL;

	memset(ipc, 0, sizeof(ipc_task_t));
	proto_init(&ipc->data);
	ipc->uid = _ipc_uid;
	ipc->state = IPC_BUSY;
	ipc->client_pid = client_proc->info.pid;
	ipc->client_uuid = client_proc->info.uuid;
	ipc->call_id = call_id;
	if(data != NULL) {
		proto_copy(&ipc->data, data->data, data->size); 
	}

	if(serv_proc->space->ipc_server.ctask == NULL) 
		serv_proc->space->ipc_server.ctask = ipc; //set current task
	else 
		queue_push(&serv_proc->space->ipc_server.tasks, ipc); // buffered
	return ipc; 
}

void proc_ipc_close(proc_t* serv_proc, ipc_task_t* ipc) {
	(void)serv_proc;
	if(ipc == NULL)
		return;

	if(serv_proc->space->ipc_server.ctask == ipc)
		serv_proc->space->ipc_server.ctask = NULL;
	ipc_free(ipc);
}

void proc_ipc_clear(proc_t* serv_proc) {
	proc_ipc_close(serv_proc, serv_proc->space->ipc_server.ctask);
	while(true) {
		ipc_task_t* ipc = (ipc_task_t*)queue_pop(&serv_proc->space->ipc_server.tasks);
		if(ipc == NULL)
			break;
		ipc_free(ipc);
	}
}