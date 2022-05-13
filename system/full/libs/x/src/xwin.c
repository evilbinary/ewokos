#include <x/xwin.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/vfs.h>
#include <sys/syscall.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <sys/vdevice.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif


int xwin_update_info(xwin_t* xwin, const xinfo_t* info) {
	proto_t in;
	if(xwin->g_buf != NULL) {
		xwin->g_buf = NULL;
		shm_unmap(xwin->g_shmid);
		xwin->g_shmid = -1;
	}

	PF->init(&in)->add(&in, info, sizeof(xinfo_t));
	int ret = vfs_fcntl(xwin->fd, X_CNTL_UPDATE_INFO, &in, NULL);
	PF->clear(&in);
	return ret;
}

int xwin_call_xim(xwin_t* xwin) {
	int ret = vfs_fcntl(xwin->fd, X_CNTL_CALL_XIM, NULL, NULL);
	return ret;
}

static int  x_get_workspace(int xfd, int style, grect_t* frame, grect_t* workspace) {
	proto_t in, out;
	PF->init(&out);

	PF->init(&in)->addi(&in, style)->add(&in, frame, sizeof(grect_t));
	int ret = vfs_fcntl(xfd, X_CNTL_WORKSPACE, &in, &out);
	PF->clear(&in);
	if(ret == 0) 
		proto_read_to(&out, workspace, sizeof(grect_t));
	PF->clear(&out);
	return ret;
}

xwin_t* xwin_open(x_t* xp, int x, int y, int w, int h, const char* title, int style) {
	if(w <= 0 || h <= 0)
		return NULL;

	int fd = open("/dev/x", O_RDWR);
	if(fd < 0)
		return NULL;

	grect_t r;
	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;
	x_get_workspace(fd, style, &r, &r);

	xwin_t* ret = (xwin_t*)malloc(sizeof(xwin_t));
	memset(ret, 0, sizeof(xwin_t));
	ret->fd = fd;

	ret->x = xp;
	if(xp->main_win == NULL)
		xp->main_win = ret;

	xinfo_t xinfo;
	memset(&xinfo, 0, sizeof(xinfo_t));
	xinfo.win = (uint32_t)ret;
	xinfo.style = style;
	xinfo.state = X_STATE_NORMAL;
	memcpy(&xinfo.wsr, &r, sizeof(grect_t));
	strncpy(xinfo.title, title, X_TITLE_MAX-1);
	xwin_update_info(ret, &xinfo);
	return ret;
}

int xwin_get_info(xwin_t* xwin, xinfo_t* info) {
	if(xwin == NULL || info == NULL)
		return -1;
	
	proto_t out;
	PF->init(&out);
	if(vfs_fcntl(xwin->fd, X_CNTL_GET_INFO, NULL, &out) != 0)
		return -1;
	proto_read_to(&out, info, sizeof(xinfo_t));
	PF->clear(&out);
	return 0;
}

static graph_t* x_get_graph(xwin_t* xwin, graph_t* g) {
	if(xwin == NULL)
		return NULL;

	xinfo_t info;
	if(xwin_get_info(xwin, &info) != 0)
		return NULL;
	if(xwin->g_buf == NULL) {
		xwin->g_buf = shm_map(info.shm_id);
		if(xwin->g_buf == NULL)
			return NULL;
		xwin->g_shmid = info.shm_id;
	}

	g->buffer = xwin->g_buf;
	g->w = info.wsr.w;
	g->h = info.wsr.h;
	g->need_free = false;
	return g;
}

void xwin_close(xwin_t* xwin) {
	if(xwin == NULL)
		return;
	if(xwin->on_close)
		xwin->on_close(xwin);

	if(xwin->g_buf != NULL)
		shm_unmap(xwin->g_shmid);

	close(xwin->fd);
	if(xwin->x->main_win == xwin)
		xwin->x->terminated = true;
	free(xwin);
}

static bool ready_to_paint(xwin_t* xwin) {
	uint32_t i = 1000;
	proto_t res;	
	PF->init(&res);
	bool ready = false;
	while(i > 0) {
		if(vfs_fcntl(xwin->fd, X_CNTL_IS_READY, NULL, &res) == 0) {
			ready = proto_read_int(&res);
			if(ready)
				break;
		}
		PF->clear(&res);
		usleep(1000);
		i--;
	}
	PF->clear(&res);
	return ready;
}

void xwin_repaint(xwin_t* xwin) {
	if(xwin->on_repaint == NULL)
		return;
	if(!ready_to_paint(xwin))
		return;

	graph_t g;
	if(x_get_graph(xwin, &g) != NULL) {
		xwin->on_repaint(xwin, &g);
		vfs_fcntl(xwin->fd, X_CNTL_UPDATE, NULL, NULL);
	}
}

void xwin_repaint_req(xwin_t* xwin) {
	x_t* x = xwin->x;
	xevent_t ev;
	memset(&ev, 0, sizeof(xevent_t));
	ev.win = (uint32_t)xwin;
	ev.value.window.event = XEVT_WIN_REPAINT;
	ev.type = XEVT_WIN;
	ipc_disable();
	x_push_event(x, &ev);
	ipc_enable();
}

int xwin_set_display(xwin_t* xwin, uint32_t display_index) {
	xinfo_t xinfo;
	xwin_get_info(xwin, &xinfo);
	xinfo.display_index = display_index;
	xwin_update_info(xwin, &xinfo);
	return 0;
}

int xwin_resize_to(xwin_t* xwin, int w, int h) {
	xinfo_t xinfo;
	xwin_get_info(xwin, &xinfo);
	xinfo.wsr.w = w;
	xinfo.wsr.h = h;
	xwin_update_info(xwin, &xinfo);
	if(xwin->on_resize) {
		xwin->on_resize(xwin);
	}
	xwin_repaint(xwin);
	return 0;
}

int xwin_resize(xwin_t* xwin, int dw, int dh) {
	xinfo_t xinfo;
	xwin_get_info(xwin, &xinfo);
	return xwin_resize_to(xwin, xinfo.wsr.w+dw, xinfo.wsr.h+dh);
}

int xwin_move_to(xwin_t* xwin, int x, int y) {
	xinfo_t xinfo;
	xwin_get_info(xwin, &xinfo);
	xinfo.wsr.x = x;
	xinfo.wsr.y = y;
	xwin_update_info(xwin, &xinfo);
	return 0;
}

int xwin_move(xwin_t* xwin, int dx, int dy) {
	xinfo_t xinfo;
	xwin_get_info(xwin, &xinfo);
	return xwin_move_to(xwin, xinfo.wsr.x+dx, xinfo.wsr.y+dy);
}

int xwin_event_handle(xwin_t* xwin, xevent_t* ev) {
	xinfo_t xinfo;
	xwin_get_info(xwin, &xinfo);

	if(ev->value.window.event == XEVT_WIN_CLOSE) {
		if(xwin->x->main_win == xwin)
			xwin->x->terminated = true;
	}
	else if(ev->value.window.event == XEVT_WIN_FOCUS) {
		if(xwin->on_focus) {
			xwin->on_focus(xwin);
		}
	}
	else if(ev->value.window.event == XEVT_WIN_UNFOCUS) {
		if(xwin->on_unfocus) {
			xwin->on_unfocus(xwin);
		}
	}
	else if(ev->value.window.event == XEVT_WIN_RESIZE) {
		xinfo.wsr.w += ev->value.window.v0;
		xinfo.wsr.h += ev->value.window.v1;
		xwin_update_info(xwin, &xinfo);
		if(xwin->on_resize) {
			xwin->on_resize(xwin);
		}
		xwin_repaint(xwin);
	}
	else if(ev->value.window.event == XEVT_WIN_MOVE) {
		xinfo.wsr.x += ev->value.window.v0;
		xinfo.wsr.y += ev->value.window.v1;
		xwin_update_info(xwin, &xinfo);
	}
	else if(ev->value.window.event == XEVT_WIN_VISIBLE) {
		xwin_set_visible(xwin, ev->value.window.v0 == 1);
	}
	else if(ev->value.window.event == XEVT_WIN_REPAINT) {
		xwin_repaint(xwin);
	}
	else if(ev->value.window.event == XEVT_WIN_MAX) {
		if(xinfo.state == X_STATE_MAX) {
			memcpy(&xinfo.wsr, &xwin->xinfo_prev.wsr, sizeof(grect_t));
			xinfo.state = xwin->xinfo_prev.state;
		}
		else {
			xscreen_t scr;
			if(x_screen_info(&scr, xinfo.display_index) == 0) {
				memcpy(&xwin->xinfo_prev, &xinfo, sizeof(xinfo_t));
				grect_t r = {0, 0, scr.size.w, scr.size.h};
				x_get_workspace(xwin->fd, xinfo.style, &r, &r);
				memcpy(&xinfo.wsr, &r, sizeof(grect_t));
				xinfo.state = X_STATE_MAX;
			}
		}
		xwin_update_info(xwin, &xinfo);
		if(xwin->on_resize) {
			xwin->on_resize(xwin);
		}
		xwin_repaint(xwin);
	}
	return 0;
}

int xwin_set_visible(xwin_t* xwin, bool visible) {
	proto_t in;
	PF->init(&in)->addi(&in, visible);

	int res = vfs_fcntl(xwin->fd, X_CNTL_SET_VISIBLE, &in, NULL);
	PF->clear(&in);
	if(xwin->on_focus)
		xwin->on_focus(xwin);
	xwin_repaint(xwin);
	return res;
}

#ifdef __cplusplus
}
#endif

