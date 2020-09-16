#include "arch/arm/bcm283x/gpio.h"
#include "arch/arm/bcm283x/spi.h"
#include <sys/vdevice.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>

#define TP_CS 7 //GPIO 7
#define TP_IRQ 25 //GPIO 25

#define SPI_CLK_DIVIDE_TEST 16384

static bool _down = false;
static int32_t _x, _y;

void TP_init(void) {
	_down = false;
	_x = _y = 0;
	spi_init(SPI_CLK_DIVIDE_TEST);
	spi_select(SPI_SELECT_0);

	gpio_config(TP_CS, GPIO_OUTPUT);
	gpio_write(TP_CS, 1); // prevent blockage of the SPI bus
	gpio_config(TP_IRQ, GPIO_INPUT);
}

static uint32_t cmd(uint8_t set_val) {
	uint16_t get_val;
	uint8_t* p = (uint8_t*)&get_val;
	spi_activate(1);
	gpio_write(TP_CS, 0);
	spi_transfer(set_val);
	p[1] = spi_transfer(0);
	p[0] = spi_transfer(0);
	gpio_write(TP_CS, 1);
	spi_activate(0);
	uint32_t ret = get_val >> 4;
	return ret;
}

static bool do_read(uint32_t* x, uint32_t* y){
	uint32_t tx[3];
	uint32_t ty[3];
	uint32_t i=0;
	for(i=0; i<3; i++){
		tx[i] =  cmd(0x90); //x
		ty[i] = cmd(0xD0);  //y
	}
	*x = (tx[0] + tx[1] + tx[2])/3;
	*y = (ty[0] + ty[1] + ty[2])/3;
	return true;
}

static int tp_read(int fd, int from_pid, fsinfo_t* info,
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)offset;
	(void)p;

	if(size < 6)
    return ERR_RETRY;

	uint32_t t = gpio_read(TP_IRQ);
	if(t == 1 && !_down)
    return ERR_RETRY;

	uint16_t* d = (uint16_t*)buf;
	if(t == 0) { //press down
		_down = true;
		d[0] = 1;

		uint32_t x, y;
		do_read(&x, &y);
		_x = x;
		_y = y;
	}
	else {  //release
		_down = false;
		d[0] = 0;
	}

	d[1] = _x;
	d[2] = _y;
	return 6;	
}

int main(int argc, char** argv) {
	TP_init();
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/touch0";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "xpt2046");
	dev.read = tp_read;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
