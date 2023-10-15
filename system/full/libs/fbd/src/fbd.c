#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/syscall.h>
#include <sys/vdevice.h>
#include <sys/shm.h>
#include <fb/fb.h>
#include <fbd/fbd.h>
#include <upng/upng.h>

typedef struct {
	uint32_t size;
	uint8_t* shm;
} fb_dma_t;

static fbinfo_t* _fbinfo = NULL;
static int32_t _rotate = 0;
static fbd_t* _fbd = NULL;

static int fb_fcntl(int fd, 
		int from_pid,
		fsinfo_t* info, 
		int cmd, 
		proto_t* in, 
		proto_t* out,
		void* p) {

	(void)fd;
	(void)from_pid;
	(void)info;
	(void)in;
	(void)p;
	if(cmd == FB_CNTL_GET_INFO) { //get fb size
		if(_rotate == G_ROTATE_N90 || _rotate == G_ROTATE_90)
			PF->addi(out, _fbinfo->height)->addi(out, _fbinfo->width)->addi(out, _fbinfo->depth);
		else
			PF->addi(out, _fbinfo->width)->addi(out, _fbinfo->height)->addi(out, _fbinfo->depth);
	}
	return 0;
}

/*static void draw_bg(graph_t* g) {
	int y, h, l;
	uint32_t c, bc;

	l = g->h/8;
	h = (g->h / l);
	h = (h==0 ? 1:h); 

	bc = 0xff / l;
	bc = (bc==0 ? 1:bc); 
	for(y=0; y<l; y++) {
		c = (l-1-y) * bc;
		graph_fill(g, 0, y*h, g->w, h, (c | c<<8 | c<<16 | 0xff000000));
	}
}
*/

static void draw_bg(graph_t* g) {
	int sz = 2; 
	int x = 0;
	int y = 0;
	bool shift = false;

	graph_clear(g, 0xffdddddd);

	for(int i=0; y<g->h; i++) {
		for(int j=0; x<g->w;j++) {
			graph_fill(g, x, y, sz, sz, 0xff555555);
			x += sz*2;
		}
		x = shift ? 0:sz;
		shift = !shift;
		y += sz;
	}
}

static void default_splash(graph_t* g) {
	draw_bg(g);
	graph_t* logo = png_image_new("/data/images/splash.png");
	if(logo != NULL) {
		graph_blt_alpha(logo, 0, 0, logo->w, logo->h,
				g, (g->w-logo->w)/2, (g->h-logo->h)/2, logo->w, logo->h, 0xff);
		graph_free(logo);
	}
}

static void init_graph(fb_dma_t* dma) {
	graph_t g;
	if(_rotate == G_ROTATE_N90 || _rotate == G_ROTATE_90)
		graph_init(&g, dma->shm, _fbinfo->height, _fbinfo->width);
	else
		graph_init(&g, dma->shm, _fbinfo->width, _fbinfo->height);

	if(_fbd->splash != NULL)
		_fbd->splash(&g);
	else
		default_splash(&g);
	_fbd->flush(_fbinfo, dma->shm, dma->size, _rotate);
}

static int fb_dma_init(fb_dma_t* dma) {
	memset(dma, 0, sizeof(fb_dma_t));
	uint32_t sz = _fbinfo->width*_fbinfo->height*4;
	dma->shm = (void*)shm_alloc(sz + 1, SHM_PUBLIC); //one more byte (head) for busy flag 
	if(dma->shm == NULL)
		return -1;
	//dma->size = _fbinfo->size_max;
	memset(dma->shm, 0, sz+1);
	dma->size = sz;
	init_graph(dma);
	return 0;
}

static int fb_dev_cntl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)from_pid;
	(void)ret;
	(void)p;

	if(cmd == FB_DEV_CNTL_SET_INFO) { //set fb size and bpp
		int w = proto_read_int(in);
		int h = proto_read_int(in);
		int bpp = proto_read_int(in);
		if(_fbd->init(w, h, bpp) != 0)
			return -1;
		_fbinfo = _fbd->get_info();
	}
	else if(cmd == FB_DEV_CNTL_GET_INFO) {
		if(_rotate == G_ROTATE_N90 || _rotate == G_ROTATE_90)
			PF->addi(ret, _fbinfo->height)->addi(ret, _fbinfo->width)->addi(ret, _fbinfo->depth);
		else
			PF->addi(ret, _fbinfo->width)->addi(ret, _fbinfo->height)->addi(ret, _fbinfo->depth);
	}	
	return 0;
}

static int32_t do_flush(fb_dma_t* dma) {
	uint8_t* buf = (uint8_t*)dma->shm;
	if(buf == NULL)
		return -1;

	uint32_t size = dma->size;
	buf[size] = 1; //busy
	int32_t res = (int32_t)_fbd->flush(_fbinfo, buf, size, _rotate);
	buf[size] = 0; //done
	return res;
}

/*return
0: error;
-1: resized;
>0: size flushed*/
static int do_fb_flush(int fd, int from_pid, fsinfo_t* info, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	fb_dma_t* dma = (fb_dma_t*)p;
	return do_flush(dma);
}

static void* fb_dma(int fd, int from_pid, fsinfo_t* info, int* size, void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	fb_dma_t* dma = (fb_dma_t*)p;
	*size = dma->size;
	return dma->shm;
}

int fbd_run(fbd_t* fbd, const char* mnt_name, uint32_t w, uint32_t h, uint32_t rotate) {
	_fbd = fbd;
	_rotate = 0;
	_rotate = rotate;

	fb_dma_t dma;
	dma.shm = NULL;
	if(fbd->init(w, h, 32) != 0)
		return -1;
	_fbinfo = fbd->get_info();
	
	if(fb_dma_init(&dma) != 0)
		return -1;

	syscall0(SYS_CLOSE_KCONSOLE);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "framebuffer");
	dev.dma = fb_dma;
	dev.flush = do_fb_flush;
	dev.fcntl = fb_fcntl;
	dev.dev_cntl = fb_dev_cntl;

	dev.extra_data = &dma;
	device_run(&dev, mnt_name, FS_TYPE_CHAR);
	shm_unmap(dma.shm);
	return 0;
}
