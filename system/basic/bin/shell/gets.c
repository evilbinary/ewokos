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
#include <sys/keydef.h>
#include "shell.h"

void add_history(const char* cmd) {
	if(_history != NULL && strcmp(cmd, _history->cmd->cstr) == 0)
		return;

	old_cmd_t* oc = (old_cmd_t*)malloc(sizeof(old_cmd_t));	
	oc->cmd = str_new(cmd);
	oc->prev = NULL;
	oc->next = _history;
	if(_history != NULL)
		_history->prev = oc;
	else
		_history_tail = oc;
	_history = oc;
}

void free_history(void) {
	old_cmd_t* oc = _history_tail;
	while(oc != NULL) {
		old_cmd_t* prev = oc->prev;
		str_free(oc->cmd);
		free(oc);
		oc = prev;
	}
}

static void clear_buf(str_t* buf) {
	if(buf->len == 0)
		return;

	uint32_t i = 0;
	while(i < buf->len) {
		buf->cstr[i] = CONSOLE_LEFT; 
		i++;
	}
	if(buf->len > 0)
		puts(buf->cstr);
	buf->len = 0;
}

int32_t gets(int fd, str_t* buf) {
	str_reset(buf);	
	old_cmd_t* head = NULL;
	old_cmd_t* tail = NULL;
	bool first_up = true;
	bool echo = true;

	while(1) {
		char c;
		int i = read(fd, &c, 1);
		if(i <= 0 || c == 0) {
		 	if(i == 0)
			 	return -1;
			usleep(10000);
			continue;
		}

		if (c == KEY_BACKSPACE) {
			if (buf->len > 0) {
				//delete last char
				putch(CONSOLE_LEFT); 
				putch(' ');
				putch(CONSOLE_LEFT); 
				buf->len--;
			}
		}
		else if (c == CONSOLE_LEFT) {
			if (buf->len > 0) {
				//delete last char
				putch(c); 
				buf->len--;
			}
		}
		else if (c == KEY_UP) { //prev command
			if(first_up) {
				head = _history;
				first_up = false;
			}
			else if(head != NULL) {
				head = head->next;
			}

			if(head != NULL) {
				tail = head;
				clear_buf(buf);
				str_cpy(buf, head->cmd->cstr);
				if(buf->len > 0)
					puts(buf->cstr);
			}
		}
		else if (c == KEY_DOWN) { //next command
			if(tail != NULL)
				tail = tail->prev;
			clear_buf(buf);

			if(tail != NULL) {
				head = tail;
				str_cpy(buf, tail->cmd->cstr);
				if(buf->len > 0)
					puts(buf->cstr);
			}
			else {
				head = _history;
				first_up = true;
			}
		}
		else {
			if(buf->len == 0 && (c == '@' || c == '#'))
				echo = false;
			if(echo && !_initrd) 
				putch(c);
			if(c == '\r' || c == '\n')
				break;
			if(c > 27)
				str_addc(buf, c);
		}
	}
	str_addc(buf, 0);
	return 0;
}
