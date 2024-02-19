#ifndef VFSCLIENT_H
#define VFSCLIENT_H

#include <ewoksys/fsinfo.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
	VFS_GET_BY_NAME = 0,
	VFS_GET_BY_NODE,
	VFS_GET_BY_FD,
	VFS_SET_FSINFO,
	VFS_NEW_NODE,
	VFS_OPEN,
	VFS_PIPE_OPEN,
	VFS_PIPE_WRITE,
	VFS_PIPE_READ,
	VFS_DUP,
	VFS_DUP2,
	VFS_CLOSE,
	VFS_DEL_NODE,
	VFS_MOUNT,
	VFS_UMOUNT,
	VFS_GET_MOUNT_BY_ID,
	VFS_GET_KIDS,
	VFS_PROC_CLONE,
	VFS_PROC_EXIT
};

int  vfs_check_access(int pid, fsinfo_t* info, int mode);

#ifdef __cplusplus
}
#endif

#endif
