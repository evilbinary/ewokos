#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/vfs.h>
#include <sys/core.h>
#include <sys/ipc.h>
#include <sys/proc.h>
#include <vprintf.h>
#include <sys/mstr.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/keydef.h>
#include <sys/klog.h>
#include "shell.h"

bool _initrd = false;
bool _terminated = false;

old_cmd_t* _history = NULL;
old_cmd_t* _history_tail = NULL;


#define ENV_PATH "PATH"

static int32_t find_exec(char* fname, char* cmd) {
	fname[0] = 0;
	fsinfo_t info;
	//get the cmd file name(without arguments).
	int32_t i = 0;	
	int32_t at = -1;
	char c = 0;
	while(cmd[i] != 0) {
		if(cmd[i] == ' ') {
			c = ' ';
			cmd[i] = 0;
			break;
		}
		i++;
	}
	at = i;
	//if cmd file is fullpath.
	if(cmd[0] == '/') {
		strcpy(fname, cmd);
		if(vfs_get(fname, &info) == 0 && info.type == FS_TYPE_FILE) {
			cmd[at] = c;
			strcpy(fname, cmd);
			return 0;
		}
	}
	//search executable file in PATH dirs.
	const char* paths = getenv(ENV_PATH);
	char path[FS_FULL_NAME_MAX];
	i = 0;
	while(1) {
		if(paths[i] == 0 || paths[i] == ':') {
			strncpy(path, paths, i);
			path[i] = 0;
			if(path[0] != 0) {
				snprintf(fname, FS_FULL_NAME_MAX-1, "%s/%s", path, cmd);
				if(vfs_get(fname, &info) == 0 && info.type == FS_TYPE_FILE) {
					cmd[at] = c;
					snprintf(fname, FS_FULL_NAME_MAX-1, "%s/%s", path, cmd);
					return 0;
				}
			}
			if(paths[i] == 0)
				break;
			paths = paths+i+1;
			i = 0;
			fname[0] = 0;
		}
		++i;
	}
	cmd[at] = c;
	return -1;
}

static void redir(const char* fname, int in) {
	while(*fname == ' ')
		fname++;

	if(in != 0) {
		int32_t fd = open(fname, O_RDONLY);
		if(fd < 0) {
			printf("error: '%s' open failed!\n", fname);
			exit(-1);
		}
		dup2(fd, 0);
		close(fd);
	}
	else {
		int32_t fd = open(fname, O_WRONLY | O_CREAT);
		if(fd < 0) {
			printf("error: '%s' open failed!\n", fname);
			exit(-1);
		}
		dup2(fd, 1);
		close(fd);
	}
}

static int do_cmd(char* cmd) {
	while(*cmd == ' ')
		cmd++;

	char fname[FS_FULL_NAME_MAX];
	if(find_exec(fname, cmd) != 0) {
		printf("'%s' not found!\n", cmd);
		return -1;
	}
	exec(fname);
	return 0;
}

static int run_cmd(char* cmd);
static int do_pipe_cmd(char* p1, char* p2) {
	int fds[2];
	if(pipe(fds) != 0) {
		printf("pipe create failed!\n");
		return -1;
	}

	int pid = fork();
	if(pid != 0) { //father proc for p2 reader.
		close(fds[1]);
		dup2(fds[0], 0);
		close(fds[0]);
		run_cmd(p2);
		exit(0);
	}
	//child proc for p1 writer
	close(fds[0]);
	dup2(fds[1], 1);
	close(fds[1]);
	return do_cmd(p1);
}

static int run_cmd(char* cmd) {
	char* proc = NULL;
	while(*cmd != 0) {
		char c = *cmd++;
		if(proc == NULL && c == ' ')
			continue;

		if(c == '>') { //redirection
			*(cmd-1) = 0;	
			redir(cmd, 0); //redir OUT.
			return do_cmd(proc);
		}
		else if(c == '<') { //redirection
			*(cmd-1) = 0;	
			redir(cmd, 1); //redir in.
			return do_cmd(proc);
		}
		else if(c == '|') { //pipe
			*(cmd-1) = 0;	
			return do_pipe_cmd(proc, cmd);
		}
		else if(proc == NULL)
			proc = cmd-1;
	}

	if(proc != NULL)
		do_cmd(proc);
	return 0;
}

static void prompt(void) {
	int uid = getuid();
	const char* cid = getenv("CONSOLE_ID");
	if(cid[0] == 0)
		cid = "0";
	char cwd[FS_FULL_NAME_MAX+1];
	if(uid == 0)
		printf("ewok(%s):%s# ", cid, getcwd(cwd, FS_FULL_NAME_MAX));
	else
		printf("ewok(%s):%s$ ", cid, getcwd(cwd, FS_FULL_NAME_MAX));
}

static void initrd_out(const char* cmd) {
	if(!_initrd || cmd[0] == '@')
		return;

	if(write(1, cmd, strlen(cmd)) > 0)
		write(1, "\n", 1);
	else
		klog("%s\n", cmd);

	if(write(2, cmd, strlen(cmd)) > 0)
		write(2, "\n", 1);
}

int main(int argc, char* argv[]) {
	_initrd = false;
	_history = NULL;
	_terminated = 0;

	int fd_in = 0;
	if(argc > 2) {
		if(strcmp(argv[1], "-initrd") == 0) {
			_initrd = true;
		}
		fd_in = open(argv[2], O_RDONLY);
		if(fd_in < 0)
			fd_in = 0;
	}

	setenv("PATH", "/sbin:/bin:/bin/x");

	const char* home = getenv("HOME");
	if(home[0] == 0)
		home = "/";
	chdir(home);

	str_t* cmdstr = str_new("");
	while(_terminated == 0) {
		if(fd_in == 0)
			prompt();

		if(gets(fd_in, cmdstr) != 0 && cmdstr->len == 0)
			break;

		char* cmd = cmdstr->cstr;
		if(cmd[0] == 0 || cmd[0] == '#')
			continue;
		initrd_out(cmd);
		
		if(cmd[0] == '@')
			cmd++;

		add_history(cmdstr->cstr);

		if(handle_shell_cmd(cmd) == 0)
			continue;

		int len = strlen(cmd)-1;
		int fg = 1;
		if(cmd[len] == '&') {
			cmd[len] = 0;
			fg = 0;
		}	

		int child_pid = fork();
		if (child_pid == 0) {
			if(fg == 0)
				proc_detach();
			int res = run_cmd(cmd);
			str_free(cmdstr);	
			return res;
		}
		else if(fg != 0) {
			waitpid(child_pid);
		}
	}

	if(fd_in > 0)
		close(fd_in);
	str_free(cmdstr);	
	free_history();
	return 0;
}
