#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/interrupt.h>
#include <sys/timer.h>
#include <pthread.h>

static uint32_t _v;

pthread_mutex_t _lock;

static void* timer_handle(void* p) {
	while(1) {
		pthread_mutex_lock(&_lock);
		_v++;
		printf("child thread: pid: %d, v: %d\n", pthread_self(), _v);
		pthread_mutex_unlock(&_lock);
		usleep(5000);
	}
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	_v = 0;

	pthread_t tid;
	pthread_create(&tid, NULL, timer_handle, NULL);
	pthread_mutex_init(&_lock, NULL);
	
	while(_v < 100) {
		pthread_mutex_lock(&_lock);
		_v++;
		printf("main thread: pid: %d, v: %d\n", pthread_self(), _v);
		pthread_mutex_unlock(&_lock);
		usleep(5000);
	}
	//pthread_mutex_destroy(&_lock);
	return 0;
}
