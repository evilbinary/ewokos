#include <unistd.h>
#include <ewoksys/vfs.h>

int dup2(int from, int to) {
	return vfs_dup2(from, to);
}

