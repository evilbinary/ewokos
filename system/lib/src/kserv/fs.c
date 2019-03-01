#include <kserv/fs.h>
#include <pmessage.h>
#include <malloc.h>
#include <kstring.h>
#include <syscall.h>
#include <stdlib.h>
#include <proto.h>

static int _fsPid = -1;

#define CHECK_KSERV_FS \
	if(_fsPid < 0) { \
		_fsPid = syscall1(SYSCALL_KSERV_GET, (int)KSERV_FS_NAME); \
		if(_fsPid < 0) \
			return -1; \
	}


int fsOpen(const char* name, int32_t flags) {
	(void) flags;
	CHECK_KSERV_FS
	int fd = -1;

	PackageT* pkg = preq(_fsPid, FS_OPEN, (void*)name, strlen(name)+1, true);
	if(pkg == NULL)	
		return -1;

	fd = *(int*)getPackageData(pkg);
	free(pkg);
	return fd;
}

int fsFInfo(const char* name, FSInfoT* info) {
	CHECK_KSERV_FS

	if(name == NULL || name[0] == 0)
		return -1;
	
	PackageT* pkg = preq(_fsPid, FS_FINFO, (void*)name, strlen(name)+1, true);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR)
		return -1;
	
	memcpy(info, getPackageData(pkg), sizeof(FSInfoT));
	free(pkg);
	return 0;
}


int fsClose(int fd) {
	CHECK_KSERV_FS

	if(fd < 0)
		return -1;

	preq(_fsPid, FS_CLOSE, (void*)&fd, 4, false);
	return 0;
}

int fsRead(int fd, char* buf, uint32_t size) {
	CHECK_KSERV_FS

	if(fd < 0)
		return -1;
	
	char req[8];
	memcpy(req, &fd, 4);
	memcpy(req+4, &size, 4);

	PackageT* pkg = preq(_fsPid, FS_READ, req, 8, true);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR)
		return -1;

	int sz = pkg->size;
	if(sz == 0) {
		free(pkg);
		return 0;
	}
	
	memcpy(buf, getPackageData(pkg), sz);
	free(pkg);
	return sz;
}

int fsWrite(int fd, const char* buf, uint32_t size) {
	CHECK_KSERV_FS

	if(fd < 0)
		return -1;
	
	char *req = (char*)malloc(size + 8);
	memcpy(req, &fd, 4);
	memcpy(req+4, &size, 4);
	memcpy(req+8, buf, size);

	PackageT* pkg = preq(_fsPid, FS_WRITE, req, size+8, true);
	free(req);

	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		return -1;
	}

	int sz = *(int*)getPackageData(pkg);
	free(pkg);
	return sz;
}

int fsAdd(int dirFD, const char* name) {
	CHECK_KSERV_FS

	int size = strlen(name);
	if(dirFD < 0 || size == 0)
		return -1;
	
	char *req = (char*)malloc(size + 8);
	memcpy(req, &dirFD, 4);
	memcpy(req+4, &size, 4);
	memcpy(req+8, name, size);

	PackageT* pkg = preq(_fsPid, FS_ADD, req, size+8, true);
	free(req);

	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		return -1;
	}

	int sz = *(int*)getPackageData(pkg);
	free(pkg);
	return sz;
}


int fsGetch(int fd) {
	char buf[1];
	if(fsRead(fd, buf, 1) != 1)
		return 0;
	return buf[0];
}

int fsPutch(int fd, int c) {
	char buf[1];
	buf[0] = (char)c;
	return fsWrite(fd, buf, 1);
}

int fsInfo(int fd, FSInfoT* info) {
	CHECK_KSERV_FS

	if(fd < 0)
		return -1;
	
	PackageT* pkg = preq(_fsPid, FS_INFO, &fd, 4, true);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL)	free(pkg);
		return -1;
	}
	
	memcpy(info, getPackageData(pkg), sizeof(FSInfoT));
	free(pkg);
	return 0;
}

FSInfoT* fsKids(int fd, uint32_t *num) {
	if(_fsPid < 0) {
		_fsPid = syscall1(SYSCALL_KSERV_GET, (int)KSERV_FS_NAME);
		if(_fsPid < 0)
			return NULL; 
	}
	
	PackageT* pkg = preq(_fsPid, FS_CHILD, &fd, 4, true);
	if(pkg == NULL || pkg->type == PKG_TYPE_ERR) {
		if(pkg != NULL)	free(pkg);
		return NULL;
	}

	ProtoT proto;
	protoInit(&proto, getPackageData(pkg), pkg->size);
	*num = protoReadInt(&proto);
	void* p = protoRead(&proto, NULL);

	uint32_t sz = (*num) * sizeof(FSInfoT);
	FSInfoT* ret = (FSInfoT*)malloc(sz);
	if(ret != NULL) {
		memcpy(ret, p, sz);
	}
	free(pkg);
	return ret;
}

int fsInited() {
	return syscall1(SYSCALL_KSERV_GET, (int)KSERV_FS_NAME);
}
