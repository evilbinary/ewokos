#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <ewoksys/session.h>
#include <ewoksys/mstr.h>
#include <ewoksys/keydef.h>

static session_info_t* check(const char* user, const char* password) {
	static session_info_t info;
	if(session_check(user, password, &info) == 0)
		return &info;
	return NULL;
}

static void input(str_t* s, bool show) {
	str_reset(s);
	char c;
	while(true) {
		int i = read(0, &c, 1);
		if(i <= 0 || c == 0) {
		 	if(errno != EAGAIN)
			 	break;
			usleep(30000);
			continue;
		}	

		if (c == KEY_BACKSPACE) {
			if (s->len > 0) {
				//delete last char
				if(show) {
					putch(CONSOLE_LEFT);
					putch(' ');
					putch(CONSOLE_LEFT);
				}
				s->len--;
			}
		}
		else {
			if(c == '\r')
				c = '\n';

			if(show || c == '\n')
				write(1, &c, 1);
			if(c == '\n')
				break;
			str_addc(s, c);
		}
	}
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	//session_info_t* info = check("root", "");  //if root have no password, run shell directly
	session_info_t* info = NULL;
	if(info == NULL || info->cmd[0] == 0) {
		str_t* user = str_new("root");
		str_t* password = str_new("");
		printf("login: ");
		input(user, true);
		if(user->len > 0) {
			info = check(user->cstr, password->cstr); 
			if(info == NULL) {
				printf("password: ");
				input(password, false);
				info = check(user->cstr, password->cstr); 
			}
		}

		str_free(user);
		str_free(password);

		if(info == NULL || info->cmd[0] == 0)
			return -1;

		if(setuid(info->uid) != 0) {
			dprintf(3, "Error, setuid failed!\n");
			return -1;
		}
	}

	setenv("HOME", info->home);
	exec(info->cmd);
	return 0;
}
