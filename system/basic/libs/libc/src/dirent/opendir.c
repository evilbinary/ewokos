#include <dirent.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/vfs.h>

DIR* opendir(const char* name) {
	fsinfo_t info;
	if(vfs_get_by_name(name, &info) != 0)
		return NULL;
	
	uint32_t num = 0;
	fsinfo_t* kids = vfs_kids(info.node, &num);
	if(kids == NULL || num == 0)
		return NULL;

	DIR* ret = (DIR*)malloc(sizeof(DIR));
	if(ret == NULL) {
		free(kids);
		return NULL;
	}

	ret->kids = kids;
	ret->num = num;
	ret->offset = 0;
	return ret;
}

