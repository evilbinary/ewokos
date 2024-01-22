#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <sysinfo.h>
#include <ewoksys/syscall.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/charbuf.h>
#include <ewoksys/mmio.h>
#include <ewoksys/proc.h>
#include <ewoksys/ipc.h>
#include <arch/bcm283x/mini_uart.h>
#include <arch/bcm283x/pl011_uart.h>

static charbuf_t _TxBuf;
static charbuf_t _RxBuf;
static bool _mini_uart;
static bool _no_return;

static int uart_read(int fd, int from_pid, fsinfo_t* node, 
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)node;
	(void)size;
	(void)p;

	int i;
	for(i = 0; i < size; i++){
	int res = charbuf_pop(&_RxBuf, buf + i);
	if(res != 0)
		break;
	}
	return (i==0)?ERR_RETRY_NON_BLOCK:i;
}

static int uart_write(int fd, int from_pid, fsinfo_t* node,
		const void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)node;
	(void)from_pid;
	(void)offset;
	(void)p;

	/*int i;
	for(i = 0; i < size; i++){
		char ch = ((char*)buf)[i];
		if(ch == '\r')
			ch = '\n';

		while(true){
			if(charbuf_push(&_TxBuf, ch, false) == 0){
				break;
			} 
			usleep(100);
		};
	}
	return size;
	*/
	if(_mini_uart)
		return bcm283x_mini_uart_write(buf, size);
	else
		return bcm283x_pl011_uart_write(buf, size);
}

static int loop(void* p) {
	(void)p;
	char c;
	ipc_disable();
	if(_mini_uart) {
		while(bcm283x_mini_uart_ready_to_recv() == 0){
			c = bcm283x_mini_uart_recv();
			if(c != '\r' || !_no_return)
				charbuf_push(&_RxBuf, c, true);
		}
	}
	else {
		while(bcm283x_pl011_uart_ready_to_recv() == 0){
			c = bcm283x_pl011_uart_recv();
			if(c != '\r' || !_no_return)
				charbuf_push(&_RxBuf, c, true);
		}
	}
	ipc_enable();
	usleep(10000);
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/tty0";
	_mmio_base = mmio_map();
	_mini_uart = true;
	_no_return = false;

	charbuf_init(&_TxBuf);
	charbuf_init(&_RxBuf);

	if(argc > 2 && strcmp(argv[2], "nr") == 0)
		_no_return = true;

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));

	sys_info_t sysinfo;
	syscall1(SYS_GET_SYS_INFO, (int32_t)&sysinfo);
	if(strcmp(sysinfo.machine, "raspberry-pi1") == 0 ||
			strcmp(sysinfo.machine, "raspberry-pi2b") == 0)  {
		strcpy(dev.name, "pl011_uart");
		_mini_uart = false;
	}
	else
		strcpy(dev.name, "mini_uart");

	dev.read = uart_read;
	dev.write = uart_write;
	dev.loop_step = loop;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);
	return 0;
}
