#ifndef UNISTD_H
#define UNISTD_H

#include <ewoksys/ewokdef.h>
#include <ewoksys/cmain.h>
#include <sys/stat.h>
#include <sys/errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RW_BLOCK_EVT	1

int getuid(void);
int setuid(int uid);
int getgid(void);
int setgid(int gid);
int getpid(void);

int fork(void);
int vfork(void);

unsigned int sleep(unsigned int seconds);
int usleep(unsigned int usecs);

int read(int fd, void* buf, uint32_t size);
int write(int fd, const void* buf, uint32_t size);
int unlink(const char* fname);
int lseek(int fd, uint32_t offset, int whence);

int exec(const char* cmd_line);

char* getcwd(char* buf, uint32_t size);
int chdir(const char* path);

int dup2(int from, int to);
int dup(int from);

int pipe(int fds[2]);

int chown(const char *pathname, int uid, int gid);
int access(const char* name, int mode);

#ifdef __cplusplus
}
#endif

#endif
