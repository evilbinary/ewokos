#include <x/xwin.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vfs.h>
#include <ewoksys/syscall.h>
#include <ewoksys/thread.h>
#include <ewoksys/proc.h>
#include <ewoksys/vdevice.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

static int xwin_update_info(xwin_t* xwin, uint8_t type) {
	if(xwin->xinfo == NULL)
		return -1;

	if(xwin->g_shm != NULL && (type & X_UPDATE_REBUILD) != 0) {
		shmdt(xwin->g_shm);
		xwin->g_shm = NULL;
	}

	proto_t in;
	PF->format(&in, "i,i", xwin->xinfo_shm_id, type);
	int ret = vfs_fcntl_wait(xwin->fd, XWIN_CNTL_UPDATE_INFO, &in);
	PF->clear(&in);
	return ret;
}

int xwin_call_xim(xwin_t* xwin) {
	int ret = vfs_fcntl(xwin->fd, XWIN_CNTL_CALL_XIM, NULL, NULL);
	return ret;
}

int xwin_top(xwin_t* xwin) {
	int ret = vfs_fcntl(xwin->fd, XWIN_CNTL_TOP, NULL, NULL);
	return ret;
}

static int  x_get_win_rect(int xfd, int style, grect_t* wsr, grect_t* win_space) {
	proto_t in, out;
	PF->init(&out);
	PF->format(&in, "i,m", style, wsr, sizeof(grect_t));
	int ret = vfs_fcntl(xfd, XWIN_CNTL_WORK_SPACE, &in, &out);
	PF->clear(&in);
	if(ret == 0) 
		proto_read_to(&out, win_space, sizeof(grect_t));
	PF->clear(&out);
	return ret;
}

xwin_t* xwin_open(x_t* xp, uint32_t disp_index, int x, int y, int w, int h, const char* title, int style) {
	if(w <= 0 || h <= 0)
		return NULL;

	if(disp_index >= x_get_display_num())
		disp_index = 0;

	int fd = open("/dev/x", O_RDWR);
	if(fd < 0)
		return NULL;

	grect_t r;
	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;

	xwin_t* ret = (xwin_t*)malloc(sizeof(xwin_t));
	memset(ret, 0, sizeof(xwin_t));
	ret->fd = fd;

	ret->x = xp;
	if(xp->main_win == NULL)
		xp->main_win = ret;

	key_t key = (((int32_t)ret) << 16) | proc_get_uuid(getpid());
	int32_t xinfo_shm_id = shmget(key, sizeof(xinfo_t), 0600 |IPC_CREAT|IPC_EXCL);
	if(xinfo_shm_id == -1) {
		free(ret);
		return NULL;
	}

	xinfo_t* xinfo = (xinfo_t*)shmat(xinfo_shm_id, 0, 0);
	if(xinfo == NULL) {
		free(ret);
		return NULL;
	}

	ret->xinfo_shm_id = xinfo_shm_id;
	ret->xinfo = xinfo;
	memset(ret->xinfo, 0, sizeof(xinfo_t));
	ret->xinfo->g_shm_id = -1;
	ret->xinfo->win = (uint32_t)ret;
	ret->xinfo->style = style;
	ret->xinfo->display_index = disp_index;
	memcpy(&ret->xinfo->wsr, &r, sizeof(grect_t));
	strncpy(ret->xinfo->title, title, XWIN_TITLE_MAX-1);
	xwin_update_info(ret, X_UPDATE_REBUILD | X_UPDATE_REFRESH);
	return ret;
}

static graph_t* x_get_graph(xwin_t* xwin, graph_t* g) {
	if(xwin == NULL || xwin->xinfo == NULL || xwin->xinfo->g_shm_id == -1)
		return NULL;

	if(xwin->g_shm == NULL) {
		xwin->g_shm = shmat(xwin->xinfo->g_shm_id, 0, 0);
		if(xwin->g_shm == NULL)
			return NULL;
		if(xwin->on_resize != NULL)
			xwin->on_resize(xwin);
	}

	g->buffer = xwin->g_shm;
	g->w = xwin->xinfo->wsr.w;
	g->h = xwin->xinfo->wsr.h;
	g->need_free = false;
	return g;
}

void xwin_close(xwin_t* xwin) {
	if(xwin == NULL)
		return;
	if(xwin->on_close)
		xwin->on_close(xwin);

	if(xwin->g_shm != NULL)
		shmdt(xwin->g_shm);

	if(xwin->xinfo != NULL)
		shmdt(xwin->xinfo);

	close(xwin->fd);
	if(xwin->x->main_win == xwin)
		x_terminate(xwin->x);
	free(xwin);
}

void xwin_repaint(xwin_t* xwin) {
	if(xwin->on_repaint == NULL)
		return;

	graph_t g;
	memset(&g, 0, sizeof(graph_t));

	if(!xwin->xinfo->repaint_lazy) {
		if(x_get_graph(xwin, &g) != NULL) {
			xwin->on_repaint(xwin, &g);
		}
	}	
	vfs_fcntl_wait(xwin->fd, XWIN_CNTL_UPDATE, NULL);
}

/*
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
*/

int xwin_set_display(xwin_t* xwin, uint32_t display_index) {
	if(display_index >= x_get_display_num())
		display_index = 0;

	xwin->xinfo->display_index = display_index;
	xwin_update_info(xwin, X_UPDATE_REFRESH);
	return 0;
}

int xwin_resize_to(xwin_t* xwin, int w, int h) {
	xwin->xinfo->wsr.w = w;
	xwin->xinfo->wsr.h = h;
	xwin_update_info(xwin, X_UPDATE_REBUILD | X_UPDATE_REFRESH);
	xwin_repaint(xwin);
	return 0;
}

int xwin_fullscreen(xwin_t* xwin) {
	xscreen_t scr;
	if(x_screen_info(&scr, xwin->xinfo->display_index) != 0)
		return -1;
	memcpy(&xwin->xinfo_prev, xwin->xinfo, sizeof(xinfo_t));
	int32_t dh = xwin->xinfo->winr.h - xwin->xinfo->wsr.h;
	grect_t r = {0, dh, scr.size.w, scr.size.h-dh};
	memcpy(&xwin->xinfo->wsr, &r, sizeof(grect_t));
	xwin->xinfo->state = XWIN_STATE_MAX;
	xwin_update_info(xwin, X_UPDATE_REBUILD | X_UPDATE_REFRESH);
	xwin_repaint(xwin);
	return 0;
}

int xwin_resize(xwin_t* xwin, int dw, int dh) {
	return xwin_resize_to(xwin, xwin->xinfo->wsr.w+dw, xwin->xinfo->wsr.h+dh);
}

int xwin_move_to(xwin_t* xwin, int x, int y) {
	xwin->xinfo->wsr.x = x;
	xwin->xinfo->wsr.y = y;
	xwin_update_info(xwin, X_UPDATE_REFRESH);
	xwin->on_move(xwin);
	return 0;
}

int xwin_move(xwin_t* xwin, int dx, int dy) {
	return xwin_move_to(xwin, xwin->xinfo->wsr.x+dx, xwin->xinfo->wsr.y+dy);
}

int xwin_event_handle(xwin_t* xwin, xevent_t* ev) {
	if(xwin->xinfo == NULL)
		return -1;

	if(ev->value.window.event == XEVT_WIN_CLOSE) {
		xwin_close(xwin);
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
	else if(ev->value.window.event == XEVT_WIN_REORG) {
		if(xwin->on_reorg) {
			xwin->on_reorg(xwin);
		}
	}
	else if(ev->value.window.event == XEVT_WIN_RESIZE) {
		xwin->xinfo->wsr.w += ev->value.window.v0;
		xwin->xinfo->wsr.h += ev->value.window.v1;
		xwin_update_info(xwin, X_UPDATE_REBUILD | X_UPDATE_REFRESH);
		xwin_repaint(xwin);
	}
	else if(ev->value.window.event == XEVT_WIN_MOVE) {
		xwin->xinfo->wsr.x += ev->value.window.v0;
		xwin->xinfo->wsr.y += ev->value.window.v1;
		xwin_update_info(xwin, X_UPDATE_REFRESH);
		xwin->on_move(xwin);
	}
	else if(ev->value.window.event == XEVT_WIN_VISIBLE) {
		xwin_set_visible(xwin, ev->value.window.v0 == 1);
	}
	else if(ev->value.window.event == XEVT_WIN_REPAINT) {
		xwin_repaint(xwin);
	}
	else if(ev->value.window.event == XEVT_WIN_MAX) {
		if(xwin->xinfo->state == XWIN_STATE_MAX) {
			memcpy(&xwin->xinfo->wsr, &xwin->xinfo_prev.wsr, sizeof(grect_t));
			xwin->xinfo->state = xwin->xinfo_prev.state;
		}
		else {
			xscreen_t scr;
			if(x_screen_info(&scr, xwin->xinfo->display_index) == 0) {
				memcpy(&xwin->xinfo_prev, xwin->xinfo, sizeof(xinfo_t));
				int32_t dh = xwin->xinfo->winr.h - xwin->xinfo->wsr.h;
				grect_t r = {0, dh, scr.size.w, scr.size.h-dh};
				memcpy(&xwin->xinfo->wsr, &r, sizeof(grect_t));
				xwin->xinfo->state = XWIN_STATE_MAX;
			}
		}
		xwin_update_info(xwin, X_UPDATE_REBUILD | X_UPDATE_REFRESH);
		xwin_repaint(xwin);
	}
	return 0;
}

void xwin_set_alpha(xwin_t* xwin, bool alpha) {
	if(xwin->xinfo == NULL)
		return;
	xwin->xinfo->alpha = alpha;
}

int xwin_set_visible(xwin_t* xwin, bool visible) {
	if(xwin->xinfo == NULL || xwin->xinfo->visible == visible)
		return 0;

	xwin->xinfo->visible = visible;
	int res = xwin_update_info(xwin, X_UPDATE_REFRESH);

	if(visible) {
		vfs_fcntl(xwin->fd, XWIN_CNTL_TRY_FOCUS, NULL, NULL);
		xwin_repaint(xwin);
	}
	return res;
}

#ifdef __cplusplus
}
#endif

