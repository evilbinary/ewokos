#include <unistd.h>

unsigned int sleep(unsigned int seconds) {
	proc_usleep(seconds * 1000 * 1000);
	return 0;
}

