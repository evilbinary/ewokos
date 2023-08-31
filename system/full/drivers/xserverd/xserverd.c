#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>
#include <sys/shm.h>
#include <fb/fb.h>
#include <sys/ipc.h>
#include <x/xcntl.h>
#include <x/xevent.h>
#include <x/xwm.h>
#include <sys/proc.h>
#include <sys/keydef.h>
#include <sconf/sconf.h>
#include <display/display.h>
#include "cursor.h"

#define X_EVENT_MAX 16

enum {
	X_win_DRAG_MOVE = 0,
	X_win_DRAG_RESIZE
};

typedef struct st_xwin {
	int fd;
	int from_pid;
	graph_t* g;
	graph_t* g_buf;
	xinfo_t* xinfo;
	bool dirty;

	grect_t r_title;
	grect_t r_close;
	grect_t r_min;
	grect_t r_max;
	grect_t r_resize;

	struct st_xwin *next;
	struct st_xwin *prev;
} xwin_t;

typedef struct {
	xwin_t* win; //moving or resizing;
	gpos_t old_pos;
	gpos_t pos_delta;
	uint32_t drag_state;
} x_current_t;

typedef struct {
	uint32_t win_move_alpha;
	uint32_t fps;
	bool bg_run;
	bool force_fullscreen;
	char xwm[128];
} x_conf_t;

typedef struct {
	fb_t fb;
	graph_t* g;
	graph_t* g_fb;
	bool dirty;
	bool curcor_task;
	bool need_repaint;
} x_display_t;

typedef struct {
	gpos_t down_pos;
	gpos_t last_pos;
	bool pressed; //true for down
} x_mouse_state_t;

typedef struct {
	const char* display_man;
	x_display_t displays[DISP_MAX];
	uint32_t display_num;
	uint32_t current_display;

	int xwm_pid;
	bool show_cursor;
	cursor_t cursor;

	xwin_t* win_head;
	xwin_t* win_tail;
	xwin_t* win_focus;
	xwin_t* win_launcher;
	xwin_t* win_last;

	xwin_t* win_xim;
	bool     win_xim_actived;
	x_mouse_state_t mouse_state;
	x_current_t current;
	x_conf_t config;
} x_t;

static int32_t read_config(x_t* x, const char* fname) {
	x->config.win_move_alpha = 0x88;
	x->config.fps = 60;
	x->config.bg_run = 0;
	x->config.xwm[0] = 0;

	sconf_t *conf = sconf_load(fname);	
	if(conf == NULL)
		return -1;

	const char* v = sconf_get(conf, "win_move_alpha");
	if(v[0] != 0) 
		x->config.win_move_alpha = atoi_base(v, 16);

	v = sconf_get(conf, "fps");
	if(v[0] != 0) 
		x->config.fps = atoi(v);
	
	v = sconf_get(conf, "bg_run");
	if(v[0] != 0) 
		x->config.bg_run = atoi(v);
	
	v = sconf_get(conf, "force_fullscreen");
	if(v[0] != 0) 
		x->config.force_fullscreen = atoi(v);

	v = sconf_get(conf, "xwm");
	if(v[0] != 0) 
		strncpy(x->config.xwm, v, 127);
	
	v = sconf_get(conf, "cursor");
	if(strcmp(v, "touch") == 0)
		x->cursor.type = CURSOR_TOUCH;
	else {
		x->cursor.type = CURSOR_MOUSE;
		if(strcmp(v, "none") == 0)
			x->show_cursor = false;
	}

	sconf_free(conf);
	return 0;
}

static void draw_win_frame(x_t* x, xwin_t* win) {
	x_display_t *display = &x->displays[win->xinfo->display_index];
	if((win->xinfo->style & X_STYLE_NO_FRAME) != 0 ||
			display->g == NULL)
		return;

	proto_t in;
	PF->init(&in)->
		addi(&in, (uint32_t)display->g->buffer)->
		addi(&in, display->g->w)->
		addi(&in, display->g->h)->
		add(&in, win->xinfo, sizeof(xinfo_t));
	if(win == x->win_focus)
		PF->addi(&in, 1); //top win
	else
		PF->addi(&in, 0);

	ipc_call_wait(x->xwm_pid, XWM_CNTL_DRAW_FRAME, &in);
	PF->clear(&in);
}

static void draw_desktop(x_t* x, uint32_t display_index) {
	x_display_t *display = &x->displays[display_index];
	if(display->g == NULL)
		return;

	proto_t in;
	PF->init(&in)->
		addi(&in, (uint32_t)display->g->buffer)->
		addi(&in, display->g->w)->
		addi(&in, display->g->h);

	int res = ipc_call_wait(x->xwm_pid, XWM_CNTL_DRAW_DESKTOP, &in);
	PF->clear(&in);
	if(res != 0)
		graph_fill(display->g, 0, 0, display->g->w, display->g->h, 0xff000000);
}

static void draw_drag_frame(x_t* xp, uint32_t display_index) {
	x_display_t *display = &xp->displays[display_index];
	if(display->g == NULL)
		return;

	int x = xp->current.win->xinfo->wsr.x;
	int y = xp->current.win->xinfo->wsr.y;
	int w = xp->current.win->xinfo->wsr.w;
	int h = xp->current.win->xinfo->wsr.h;

	if(xp->current.drag_state == X_win_DRAG_MOVE)  {
		x += xp->current.pos_delta.x;
		y += xp->current.pos_delta.y;
	}
	else if(xp->current.drag_state == X_win_DRAG_RESIZE)  {
		w += xp->current.pos_delta.x;
		h += xp->current.pos_delta.y;
	}

	grect_t r = {x, y, w, h};

	proto_t in;
	PF->init(&in)->
		addi(&in, (uint32_t)display->g->buffer)->
		addi(&in, display->g->w)->
		addi(&in, display->g->h)->
		add(&in, &r, sizeof(grect_t));

	ipc_call_wait(xp->xwm_pid, XWM_CNTL_DRAW_DRAG_FRAME, &in);
	PF->clear(&in);
}

static int draw_win(x_t* xp, xwin_t* win) {
	x_display_t *display = &xp->displays[win->xinfo->display_index];
	if(!display->dirty && !win->dirty)
		return -1;
	uint32_t to = 0;

	graph_t* g = win->g_buf;
	if(g != NULL) {
		if((win->xinfo->style & X_STYLE_ALPHA) != 0) {
			graph_blt_alpha(g, 0, 0, 
					win->xinfo->wsr.w,
					win->xinfo->wsr.h,
					display->g,
					win->xinfo->wsr.x,
					win->xinfo->wsr.y,
					win->xinfo->wsr.w,
					win->xinfo->wsr.h, 0xff);
		}
		else {
			graph_blt(g, 0, 0, 
						win->xinfo->wsr.w,
						win->xinfo->wsr.h,
						display->g,
						win->xinfo->wsr.x,
						win->xinfo->wsr.y,
						win->xinfo->wsr.w,
						win->xinfo->wsr.h);
		}
	}

	draw_win_frame(xp, win);
	if(xp->current.win == win && xp->config.win_move_alpha < 0xff) //drag and moving
		draw_drag_frame(xp, win->xinfo->display_index);

	win->dirty = false;
	return 0;
}

static inline void x_dirty(x_t* x, int32_t display_index) {
	if(display_index >= 0) {
		x_display_t *display = &x->displays[display_index];
		display->dirty = true;
		display->need_repaint = true;
		return;
	}

	for(uint32_t i=0; i<x->display_num; i++) {
		x_display_t *display = &x->displays[i];
		display->dirty = true;
		display->need_repaint = true;
	}
}

static void remove_win(x_t* x, xwin_t* win) {
	if(win->prev != NULL)
		win->prev->next = win->next;
	if(win->next != NULL)
		win->next->prev = win->prev;
	if(x->win_tail == win)
		x->win_tail = win->prev;
	if(x->win_head == win)
		x->win_head = win->next;
	win->next = win->prev = NULL;
	x_dirty(x, win->xinfo->display_index);
}

static inline void send_event(int32_t pid, xevent_t* e) {
	proto_t in;
	PF->init(&in)->add(&in, e, sizeof(xevent_t));
	ipc_call(pid, X_CMD_PUSH_EVENT, &in, NULL);
	PF->clear(&in);
	//proc_wakeup(pid);
}

static void x_push_event(x_t* x, xwin_t* win, xevent_t* e) {
	(void)x;
	if(win->from_pid <= 0)
		return;
	e->win = win->xinfo->win;
	send_event(win->from_pid, e);
}

static void hide_win(x_t* x, xwin_t* win) {
	x->win_xim_actived = false;
	if(win == NULL)
		return;

	xevent_t e;
	e.type = XEVT_WIN;
	e.value.window.event = XEVT_WIN_VISIBLE;
	e.value.window.v0 = 0;
	x_push_event(x, win, &e);
}

static void show_win(x_t* x, xwin_t* win) {
	if(win == NULL)
		return;

	x->win_xim_actived = true;
	xevent_t e;
	e.type = XEVT_WIN;
	e.value.window.event = XEVT_WIN_VISIBLE;
	e.value.window.v0 = 1;
	x_push_event(x, win, &e);
}

static void try_focus(x_t* x, xwin_t* win) {
	if(x->win_focus == win)
		return;
	if((win->xinfo->style & X_STYLE_NO_FOCUS) == 0) {
		hide_win(x, x->win_xim);
		if(x->win_focus != NULL) {
			xevent_t e;
			e.type = XEVT_WIN;
			e.value.window.event = XEVT_WIN_UNFOCUS;
			x_push_event(x, x->win_focus, &e);
		}

		xevent_t e;
		e.type = XEVT_WIN;
		e.value.window.event = XEVT_WIN_FOCUS;
		x_push_event(x, win, &e);
		x->win_focus = win;
	}
}

static inline void x_repaint_req(x_t* x, int32_t display_index) {
	if(display_index >= 0) {
		x_display_t *display = &x->displays[display_index];
		display->need_repaint = true;
		return;
	}

	for(uint32_t i=0; i<x->display_num; i++) {
		x_display_t *display = &x->displays[i];
		display->need_repaint = true;
	}
}

static void push_win(x_t* x, xwin_t* win) {
	if((win->xinfo->style & X_STYLE_SYSBOTTOM) != 0) { //push head if sysbottom style
		if(x->win_head != NULL) {
			x->win_head->prev = win;
			win->next = x->win_head;
			x->win_head = win;
		}
		else {
			x->win_tail = x->win_head = win;
		}
	}
	else if((win->xinfo->style & X_STYLE_SYSTOP) != 0) { //push tail if systop style
		if(x->win_tail != NULL) {
			x->win_tail->next = win;
			win->prev = x->win_tail;
			x->win_tail = win;
		}
		else {
			x->win_tail = x->win_head = win;
		}
	}
	else { 
		xwin_t* win_top = x->win_tail;
		xwin_t* win_systop = NULL;
		while(win_top != NULL) {
			if((win_top->xinfo->style & X_STYLE_SYSTOP) == 0)
				break;
			win_systop = win_top;
			win_top = win_top->prev;
		}

		if(win_top != NULL) {
			win->next = win_top->next;
			if(win_top->next != NULL)
				win_top->next->prev = win;
			else 
				x->win_tail = win;

			win_top->next = win;
			win->prev = win_top;
		}
		else {
			x->win_head = win;
			if(win_systop != NULL)  {
				if(win_systop->prev != NULL)
					win_systop->prev->next = win;

				win->prev = win_systop->prev;
				win_systop->prev = win;
				win->next = win_systop;
			}
			else {
				x->win_tail = win;
			}
		}
	}
	try_focus(x, win);
}

static xwin_t* get_next_focus_win(x_t* x, bool skip_launcher) {
	xwin_t* ret = x->win_tail; 
	while(ret != NULL) {
		if(ret->xinfo->visible &&
				(ret->xinfo->style & X_STYLE_NO_FOCUS) == 0 &&
				(!skip_launcher || ret != x->win_launcher))
			return ret;
		ret = ret->prev;
	}
	return NULL;
}

static void x_del_win(x_t* x, xwin_t* win) {
	if(win == x->win_focus)
		hide_win(x, x->win_xim);
	if(win == x->win_last)
		x->win_last = NULL;

	remove_win(x, win);

	if(win->xinfo->g_shm != NULL) {
		shm_unmap(win->xinfo->g_shm);
		shm_unmap(win->xinfo);
	}

	if(win->g_buf != NULL)
		graph_free(win->g_buf);

	free(win);
	x->win_focus = get_next_focus_win(x, false);
	x->win_last = get_next_focus_win(x, true);
	if(x->win_focus != NULL) {
		xevent_t e;
		e.type = XEVT_WIN;
		e.value.window.event = XEVT_WIN_FOCUS;
		x_push_event(x, x->win_focus, &e);
	}
}

static void hide_cursor(x_t* x) {
	x_display_t* display = &x->displays[x->current_display];
	if(x->cursor.drop || display->g == NULL)
		return;

	if(x->cursor.g == NULL) {
		x->cursor.g = graph_new(NULL, x->cursor.size.w, x->cursor.size.h);
		graph_blt(display->g,
				x->cursor.old_pos.x - x->cursor.offset.x,
				x->cursor.old_pos.y - x->cursor.offset.y,
				x->cursor.size.w,
				x->cursor.size.h,
				x->cursor.g,
				0,
				0, 
				x->cursor.size.w,
				x->cursor.size.h);
	}
	else  {
		graph_blt(x->cursor.g,
				0,
				0,
				x->cursor.size.w,
				x->cursor.size.h,
				display->g,
				x->cursor.old_pos.x - x->cursor.offset.x,
				x->cursor.old_pos.y - x->cursor.offset.x,
				x->cursor.size.w,
				x->cursor.size.h);
	}
}

static inline void refresh_cursor(x_t* x) {
	x_display_t* display = &x->displays[x->current_display];
	if(display->g == NULL || x->cursor.g == NULL)
		return;
	int32_t mx = x->cursor.cpos.x - x->cursor.offset.x;
	int32_t my = x->cursor.cpos.y - x->cursor.offset.y;
	int32_t mw = x->cursor.size.w;
	int32_t mh = x->cursor.size.h;

	graph_blt(display->g, mx, my, mw, mh,
			x->cursor.g, 0, 0, mw, mh);

	draw_cursor(display->g, &x->cursor, mx, my);

	x->cursor.old_pos.x = x->cursor.cpos.x;
	x->cursor.old_pos.y = x->cursor.cpos.y;
	x->cursor.drop = false;
}

static int x_init_display(x_t* x, int32_t display_index) {
	uint32_t display_num = get_display_num(x->display_man);
	if(display_index >= 0) {
		const char* fb_dev = get_display_fb_dev(x->display_man, display_index);
		if(fb_open(fb_dev, &x->displays[0].fb) != 0)
			return -1;
		graph_t *g_fb = fb_fetch_graph(&x->displays[0].fb);
		x->displays[0].g_fb = g_fb;
		void* p = shm_alloc(g_fb->w * g_fb->h * 4, SHM_PUBLIC);
		x->displays[0].g = graph_new(p, g_fb->w, g_fb->h);
		
		x_dirty(x, 0);
		x->display_num = 1;
		return 0;
	}

	for(uint32_t i=0; i<display_num; i++) {
		const char* fb_dev = get_display_fb_dev(x->display_man, i);
		if(fb_open(fb_dev, &x->displays[i].fb) != 0)
			return -1;
		graph_t *g_fb = fb_fetch_graph(&x->displays[i].fb);
		x->displays[i].g_fb = g_fb;
		void* p = shm_alloc(g_fb->w * g_fb->h * 4, SHM_PUBLIC);
		x->displays[i].g = graph_new(p, g_fb->w, g_fb->h);
		x_dirty(x, i);
	}
	x->display_num = display_num;
	return 0;
}

static int x_init(x_t* x, const char* display_man, int32_t display_index) {
	memset(x, 0, sizeof(x_t));
	x->xwm_pid = -1;

	x->display_man = display_man;
	if(x_init_display(x, display_index) != 0)
		return -1;

	x_display_t* display = &x->displays[0];
	x->cursor.cpos.x = display->g->w/2;
	x->cursor.cpos.y = display->g->h/2; 
	x->show_cursor = true;
	return 0;
}	


static void x_close(x_t* x) {
	for(uint32_t i=0; i<x->display_num; i++) {
		x_display_t* display = &x->displays[i];
		fb_close(&display->fb);
		if(display->g != NULL) {
			shm_unmap(display->g->buffer);
			graph_free(display->g);
		}
		if(display->g_fb != NULL) {
			shm_unmap(display->g_fb->buffer);
			graph_free(display->g_fb);
		}
	}
}

static void x_repaint(x_t* x, uint32_t display_index) {
	x_display_t* display = &x->displays[display_index];
	if(display->g == NULL ||
			!display->need_repaint ||
			fb_busy(&display->fb))
		return;

	display->need_repaint = false;
	bool do_flush = false;

	if(display->curcor_task)
		do_flush = true;

	if(x->show_cursor && x->current_display == display_index)
		hide_cursor(x);

	bool undirty = false;
	if(display->dirty) {
		draw_desktop(x, display_index);
		undirty = true;
		do_flush = true;
	}

	xwin_t* win = x->win_head;
	while(win != NULL) {
		if(win->xinfo->visible && win->xinfo->display_index == display_index) {
			if(draw_win(x, win) == 0)
				do_flush = true;
		}
		win = win->next;
	}

	if(x->current_display == display_index) {
		if(x->show_cursor)
			refresh_cursor(x);
	}
	display->dirty = false;

	if(do_flush) {
		memcpy(display->g_fb->buffer,
				display->g->buffer,
				display->g->w * display->g->h * 4);
		fb_flush(&display->fb, false);
	}
}

static xwin_t* x_get_win(x_t* x, int fd, int from_pid) {
	xwin_t* win = x->win_head;
	while(win != NULL) {
		if(win->fd == fd && win->from_pid == from_pid)
			return win;
		win = win->next;
	}
	return NULL;
}

static xwin_t* get_first_visible_win(x_t* x) {
	xwin_t* ret = x->win_tail; 
	while(ret != NULL) {
		if(ret->xinfo->visible)
			return ret;
		ret = ret->prev;
	}
	return NULL;
}

static void mark_dirty(x_t* x, xwin_t* win) {
	xwin_t* win_next = win->next;

	if(win->xinfo->visible && win->dirty) {
		xwin_t* v = win->next;
		while(v != NULL) {
			grect_t r;
			if(v->xinfo->visible && !v->dirty) {
				memcpy(&r, &v->xinfo->winr, sizeof(grect_t));
				grect_insect(&win->xinfo->winr, &r);
				if(r.x == win->xinfo->winr.x &&
						r.y == win->xinfo->winr.y &&
						r.w == win->xinfo->winr.w &&
						r.h == win->xinfo->winr.h &&
						(v->xinfo->style & X_STYLE_ALPHA) == 0) { 
					//covered by upon window. don't have to repaint.
					win->dirty = false;
					break;
				}
				else if(r.w != 0 || r.h != 0) {
					v->dirty = true;
				}
			}
			v = v->next;
		}

		if(win->dirty && (win->xinfo->style & X_STYLE_ALPHA) != 0) {
			x_dirty(x, win->xinfo->display_index);
			return;
		}
	}

	if(win_next != NULL)
		mark_dirty(x, win_next);
}

static void unmark_dirty(x_t* x, xwin_t* win) {
	(void)x;
	xwin_t* v = win->next;
	while(v != NULL) {
		v->dirty = false;
		v = v->next;
	}
}

static int x_update(int fd, int from_pid, x_t* x) {
	if(fd < 0)
		return -1;
	
	xwin_t* win = x_get_win(x, fd, from_pid);
	if(win == NULL)
		return -1;
	if(!win->xinfo->visible)
		return 0;

	if(win->g_buf == NULL)
		win->g_buf = graph_new(NULL, win->g->w, win->g->h);
	memcpy(win->g_buf->buffer, win->g->buffer, win->g->w * win->g->h * 4);
	win->dirty = true;

	mark_dirty(x, win);

	if(!win->dirty)
		unmark_dirty(x, win);
	return 0;
}

static int xwin_set_visible(int fd, int from_pid, proto_t* in, x_t* x) {
	if(fd < 0)
		return -1;
	
	xwin_t* win = x_get_win(x, fd, from_pid);
	if(win == NULL)
		return -1;

	win->xinfo->visible = proto_read_int(in);
	win->dirty = true;
	x_dirty(x, win->xinfo->display_index);
	return 0;
}

static int x_update_frame_areas(x_t* x, xwin_t* win) {
	if((win->xinfo->style & X_STYLE_NO_FRAME) != 0)
		return -1;

	proto_t in, out;
	PF->init(&out);
	PF->init(&in)->
		add(&in, win->xinfo, sizeof(xinfo_t));
	int res = ipc_call(x->xwm_pid, XWM_CNTL_GET_FRAME_AREAS, &in, &out);
	PF->clear(&in);

	proto_read_to(&out, &win->r_title, sizeof(grect_t));
	proto_read_to(&out, &win->r_close, sizeof(grect_t));
	proto_read_to(&out, &win->r_min, sizeof(grect_t));
	proto_read_to(&out, &win->r_max, sizeof(grect_t));
	proto_read_to(&out, &win->r_resize, sizeof(grect_t));
	PF->clear(&out);
	return res;
}

static void x_get_min_size(x_t* x, xwin_t* win, int *w, int* h) {
	proto_t in, out;
	PF->init(&out);
	PF->init(&in)->
		add(&in, win->xinfo, sizeof(xinfo_t));
	int res = ipc_call(x->xwm_pid, XWM_CNTL_GET_MIN_SIZE, &in, &out);
	PF->clear(&in);
	if(res == 0) { 
		*w = proto_read_int(&out);
		*h = proto_read_int(&out);
	}
	PF->clear(&out);
}

static void xwin_force_fullscreen(x_t* x, xinfo_t* xinfo) {
	if(!x->config.force_fullscreen ||
			(xinfo->style & X_STYLE_ANTI_FSCR) != 0)
		return;

	xinfo->wsr.x = 0;
	xinfo->wsr.y = 0;
	xinfo->wsr.w = x->displays[x->current_display].g->w;
	xinfo->wsr.h = x->displays[x->current_display].g->h;
	xinfo->style |= X_STYLE_NO_FRAME;
}

static int get_xwm_win_space(x_t* x, int style, grect_t* rin, grect_t* rout) {
	proto_t in, out;
	PF->init(&out);

	PF->init(&in)->
		addi(&in, style)->
		add(&in, rin, sizeof(grect_t));

	int res = ipc_call(x->xwm_pid, XWM_CNTL_GET_WIN_SPACE, &in, &out);
	PF->clear(&in);
	if(res == 0)
		proto_read_to(&out, rout, sizeof(grect_t));
	PF->clear(&out);

	return res;
}

static int xwin_update_info(int fd, int from_pid, proto_t* in, proto_t* out, x_t* x) {
	if(fd < 0)
		return -1;

	xinfo_t *xinfo = proto_read_int(in);
	if(xinfo == NULL)
		return -1;
	uint8_t type = proto_read_int(in);
	
	xwin_t* win = x_get_win(x, fd, from_pid);
	if(win == NULL)
		return -1;

	if(win->xinfo == NULL)
		win->xinfo = shm_map(xinfo);
	if(win->xinfo == NULL)
		return -1;


	if((win->xinfo->style & X_STYLE_LAUNCHER) != 0)
		x->win_launcher = win;
	if((win->xinfo->style & X_STYLE_XIM) != 0)
		x->win_xim = win;

	int wsr_w = win->xinfo->wsr.w;
	int wsr_h = win->xinfo->wsr.h;
	xwin_force_fullscreen(x, win->xinfo);
	
	if((win->xinfo->style & X_STYLE_NO_FRAME) == 0 &&
      (win->xinfo->style & X_STYLE_NO_TITLE) == 0) {
		int minw = 0, minh = 0;
		x_get_min_size(x, win, &minw, &minh);
		if(win->xinfo->wsr.w < minw)
			win->xinfo->wsr.w = minw;
		if(win->xinfo->wsr.h < minh)
			win->xinfo->wsr.h = minh;
	}
	if(wsr_w != win->xinfo->wsr.w || wsr_h != win->xinfo->wsr.h)
		type = type | X_UPDATE_REBUILD | X_UPDATE_REFRESH;
	
	if(get_xwm_win_space(x, (int)win->xinfo->style,
			&win->xinfo->wsr,
			&win->xinfo->winr) != 0)	
		return -1;
	
	void* g_shm = win->xinfo->g_shm;
	if((type & X_UPDATE_REBUILD) != 0 ||
			g_shm == NULL ||
			win->g == NULL) {
		if(win->g != NULL && g_shm != NULL) {
			graph_free(win->g);
			shm_unmap(g_shm);
			win->g = NULL;
		}
		g_shm = shm_alloc(win->xinfo->wsr.w * win->xinfo->wsr.h * 4, SHM_PUBLIC);
		if(g_shm == NULL) 
			return -1;
		win->xinfo->g_shm = g_shm;
		win->g = graph_new(g_shm, win->xinfo->wsr.w, win->xinfo->wsr.h);
		graph_clear(win->g, 0x0);

		if(win->g_buf != NULL) {
			graph_free(win->g_buf);
			win->g_buf = NULL;
		}
	}
	x_update_frame_areas(x, win);

	if((type & X_UPDATE_REFRESH) != 0 ||
			(win->xinfo->style & X_STYLE_ALPHA) != 0) {
		x_dirty(x, win->xinfo->display_index);
	}
	return 0;
}

static int x_win_space(x_t* x, proto_t* in, proto_t* out) {
	grect_t r;
	int style = proto_read_int(in);
	proto_read_to(in, &r, sizeof(grect_t));
	get_xwm_win_space(x, style, &r, &r); 
	PF->add(out, &r, sizeof(grect_t));
	return 0;
}

static int xwin_call_xim(x_t* x) {
	show_win(x, x->win_xim);
	return 0;
}

static int xserver_fcntl(int fd, int from_pid, fsinfo_t* info,
		int cmd, proto_t* in, proto_t* out, void* p) {
	(void)fd;
	(void)info;
	x_t* x = (x_t*)p;

	int res = -1;
	if(cmd == X_CNTL_UPDATE) {
		res = x_update(fd, from_pid, x);
	}	
	else if(cmd == X_CNTL_UPDATE_INFO) {
		res = xwin_update_info(fd, from_pid, in, out, x);
	}
	else if(cmd == X_CNTL_WIN_SPACE) {
		res = x_win_space(x, in, out);
	}
	else if(cmd == X_CNTL_CALL_XIM) {
		res = xwin_call_xim(x);
	}
	return res;
}

static int xserver_win_open(int fd, int from_pid, fsinfo_t* info, int oflag, void* p) {
	(void)oflag;
	(void)info;
	if(fd < 0)
		return -1;

	x_t* x = (x_t*)p;
	xwin_t* win = (xwin_t*)malloc(sizeof(xwin_t));
	if(win == NULL)
		return -1;

	memset(win, 0, sizeof(xwin_t));
	win->fd = fd;
	win->from_pid = from_pid;
	push_win(x, win);
	return 0;
}

static void mouse_cxy(x_t* x, uint32_t display_index, int32_t rx, int32_t ry) {
	x_display_t* display = &x->displays[display_index];
	x->cursor.cpos.x += rx;
	x->cursor.cpos.y += ry;

	if(x->cursor.cpos.x < 0)
		x->cursor.cpos.x = 0;

	if(x->cursor.cpos.x > (int32_t)display->g->w)
		x->cursor.cpos.x = display->g->w;

	if(x->cursor.cpos.y < 0)
		x->cursor.cpos.y = 0;

	if(x->cursor.cpos.y >= (int32_t)display->g->h)
		x->cursor.cpos.y = display->g->h;
}

enum {
	FRAME_R_TITLE = 0,
	FRAME_R_CLOSE,
	FRAME_R_MIN,
	FRAME_R_MAX,
	FRAME_R_RESIZE
};

static int get_win_frame_pos(x_t* x, xwin_t* win) {
	if((win->xinfo->style & X_STYLE_NO_FRAME) != 0)
		return -1;

	int res = -1;
	if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &win->r_title))
		res = FRAME_R_TITLE;
	else if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &win->r_close))
		res = FRAME_R_CLOSE;
	else if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &win->r_min))
		res = FRAME_R_MIN;
	else if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &win->r_max))
		res = FRAME_R_MAX;
	else if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &win->r_resize))
		res = FRAME_R_RESIZE;
	return res;
}

static xwin_t* get_mouse_owner(x_t* x, int* win_frame_pos) {
	xwin_t* win = x->win_tail;
	if(win_frame_pos != NULL)
		*win_frame_pos = -1;

	while(win != NULL) {
		if(!win->xinfo->visible || win->xinfo->display_index != x->current_display) {
			win = win->prev;
			continue;
		}
		int pos = get_win_frame_pos(x, win);
		if(pos >= 0) {
			if(win_frame_pos != NULL)
				*win_frame_pos = pos;
			return win;
		}
		if(check_in_rect(x->cursor.cpos.x, x->cursor.cpos.y, &win->xinfo->wsr))
			return win;
		win = win->prev;
	}
	return NULL;
}

static void xwin_top(x_t* x, xwin_t* win) {
	remove_win(x, win);
	push_win(x, win);
}

static void xwin_bg(x_t* x, xwin_t* win) {
	if(win == NULL)
		return;
	if(!x->config.bg_run && win != x->win_launcher) {
		xevent_t ev;
		ev.type = XEVT_WIN;
		ev.value.window.event = XEVT_WIN_CLOSE;
		x_push_event(x, win, &ev);
		return;
	}

	if(x->win_focus != x->win_launcher) {
		x->win_last = x->win_focus;
		xwin_top(x, x->win_launcher);
	}
	else if(x->win_last != NULL) {
		xwin_top(x, x->win_last);
	}
}

static void mouse_xwin_handle(x_t* x, xwin_t* win, int pos, xevent_t* ev) {
	ev->value.mouse.winx = ev->value.mouse.x - win->xinfo->wsr.x;
	ev->value.mouse.winy = ev->value.mouse.y - win->xinfo->wsr.y;

	if(ev->state ==  XEVT_MOUSE_DOWN) {
		if(win != x->win_tail) {
			xwin_top(x, win);
		}
		else {
			try_focus(x, win);
		}

		if(pos == FRAME_R_RESIZE) //window resize
			return;
	}
	else if(ev->state ==  XEVT_MOUSE_DRAG) {
		if(pos == FRAME_R_TITLE) {//window title 
			x->current.win = win;
			x->current.old_pos.x = x->cursor.cpos.x;
			x->current.old_pos.y = x->cursor.cpos.y;
			x->current.drag_state = X_win_DRAG_MOVE;
			//x_dirty(x, x->current_display);
		}
		else if(pos == FRAME_R_RESIZE) {//window resize
			x->current.win = win;
			x->current.old_pos.x = x->cursor.cpos.x;
			x->current.old_pos.y = x->cursor.cpos.y;
			x->current.drag_state = X_win_DRAG_RESIZE;
			//x_dirty(x, x->current_display);
		}
	}
	else if(ev->state == XEVT_MOUSE_UP) {
		if(pos == FRAME_R_RESIZE) //window resize
				return;

		if(x->current.win == win) {
			ev->type = XEVT_WIN;
			ev->value.window.v0 =  x->current.pos_delta.x;
			ev->value.window.v1 =  x->current.pos_delta.y;
			if(x->current.drag_state == X_win_DRAG_RESIZE) {
				ev->value.window.event = XEVT_WIN_RESIZE;
				graph_free(win->g);
				shm_unmap(win->xinfo->g_shm);
				win->g = NULL;
				win->xinfo->g_shm = NULL;
			}
			else if(x->current.drag_state == X_win_DRAG_MOVE) {
				ev->value.window.event = XEVT_WIN_MOVE;
			}
			x->current.pos_delta.x = 0;
			x->current.pos_delta.y = 0;
		}
		else if(abs_32(ev->value.mouse.from_x - ev->value.mouse.x) < 6 &&
				abs_32(ev->value.mouse.from_y - ev->value.mouse.y) < 6) {
			x_push_event(x, win, ev);
			ev->state = XEVT_MOUSE_CLICK;
		}

		if(ev->state == XEVT_MOUSE_CLICK) {
			if(pos == FRAME_R_CLOSE) { //window close
				ev->type = XEVT_WIN;
				ev->value.window.event = XEVT_WIN_CLOSE;
				//win->xinfo->visible = false;
				//x_dirty(x);
			}
			else if(pos == FRAME_R_MAX) {
				ev->type = XEVT_WIN;
				ev->value.window.event = XEVT_WIN_MAX;
			}
		}
		x->current.win = NULL;
	}

	if(x->current.win == win) {
		int mrx = x->cursor.cpos.x - x->current.old_pos.x;
		int mry = x->cursor.cpos.y - x->current.old_pos.y;
		if(abs(mrx) > 15 || abs(mry) > 15) {
			x->current.pos_delta.x = mrx;
			x->current.pos_delta.y = mry;
			x_dirty(x, x->current_display);
		}
		return; //drag win frame, don't push xwin event.
	}

	x_push_event(x, win, ev);
}

static void cursor_safe(x_t* x, x_display_t* display) {
	int margin = x->cursor.size.w - x->cursor.offset.x;
	if(x->cursor.cpos.x < x->cursor.offset.x)
		x->cursor.cpos.x = x->cursor.offset.x;
	else if(x->cursor.cpos.x > (display->g->w - margin))
		x->cursor.cpos.x = display->g->w - margin;

	margin = x->cursor.size.h - x->cursor.offset.y;
	if(x->cursor.cpos.y < x->cursor.offset.y)
		x->cursor.cpos.y = x->cursor.offset.y;
	else if(x->cursor.cpos.y > (display->g->h - margin))
		x->cursor.cpos.y = display->g->h - margin;
}

static int mouse_handle(x_t* x, xevent_t* ev) {
	if(ev->value.mouse.relative != 0) {
		mouse_cxy(x, x->current_display, ev->value.mouse.rx, ev->value.mouse.ry);
		ev->value.mouse.x = x->cursor.cpos.x;
		ev->value.mouse.y = x->cursor.cpos.y;
	}
	else {
		x->cursor.cpos.x = ev->value.mouse.x;
		x->cursor.cpos.y = ev->value.mouse.y;
	}

	x_display_t *display = &x->displays[x->current_display];
	display->curcor_task = true;
	cursor_safe(x, display);
	if(ev->state ==  XEVT_MOUSE_DOWN) {
		x->cursor.down = true;
		if(!x->mouse_state.pressed) {
			x->mouse_state.pressed = true;
			x->mouse_state.down_pos.x = ev->value.mouse.x;
			x->mouse_state.down_pos.y = ev->value.mouse.y;
			x->mouse_state.last_pos.x = ev->value.mouse.x;
			x->mouse_state.last_pos.y = ev->value.mouse.y;
			ev->value.mouse.rx = 0;
			ev->value.mouse.ry = 0;
		}
		else if(ev->value.mouse.from_x != ev->value.mouse.x ||
					ev->value.mouse.from_y != ev->value.mouse.y) {
			ev->state = XEVT_MOUSE_DRAG;
			ev->value.mouse.from_x = x->mouse_state.down_pos.x;
			ev->value.mouse.from_y = x->mouse_state.down_pos.y;
			ev->value.mouse.rx = ev->value.mouse.x - x->mouse_state.last_pos.x;
			ev->value.mouse.ry = ev->value.mouse.y - x->mouse_state.last_pos.y;
			x->mouse_state.last_pos.x = ev->value.mouse.x;
			x->mouse_state.last_pos.y = ev->value.mouse.y;
		}
		if(ev->value.mouse.from_y >= (display->g->h-16)) //from bottom
			return 0;
	}
	else if(ev->state ==  XEVT_MOUSE_UP) {
		x->cursor.down = false;
		x->mouse_state.pressed = false;
		ev->value.mouse.from_x = x->mouse_state.down_pos.x;
		ev->value.mouse.from_y = x->mouse_state.down_pos.y;
		ev->value.mouse.rx = ev->value.mouse.x - x->mouse_state.last_pos.x;
		ev->value.mouse.ry = ev->value.mouse.y - x->mouse_state.last_pos.y;
		x->mouse_state.last_pos.x = ev->value.mouse.x;
		x->mouse_state.last_pos.y = ev->value.mouse.y;

		if(ev->value.mouse.from_y >= (display->g->h-16) && //from bottom
				ev->value.mouse.from_y > ev->value.mouse.y)  { //swap up from bottom
			xwin_bg(x, x->win_focus);
			return 0;
		}
	}
	

	int pos = -1;
	xwin_t* win = NULL;
	if(x->current.win != NULL)
		win = x->current.win;
	else
		win = get_mouse_owner(x, &pos);

	if(win != NULL)
		mouse_xwin_handle(x, win, pos, ev);
	return 0;
}

static int im_handle(x_t* x, int32_t from_pid, xevent_t* ev) {
	if(ev->value.im.value == KEY_HOME && ev->state == XIM_STATE_RELEASE) {
		xwin_bg(x, x->win_focus);
		return 0;
	}

	if(x->win_xim_actived && x->win_xim != NULL && from_pid != x->win_xim->from_pid) {
		x_push_event(x, x->win_xim, ev);
	}
	else if(x->win_focus != NULL) {
		x_push_event(x, x->win_focus, ev);
	}
	return 0;
}

static void handle_input(x_t* x, int32_t from_pid, xevent_t* ev) {
	if(ev->type == XEVT_IM) {
		im_handle(x, from_pid, ev);
	}
	else if(ev->type == XEVT_MOUSE) {
		mouse_handle(x, ev);
	}
}

static int xserver_dev_cntl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)from_pid;
	x_t* x = (x_t*)p;

	if(cmd == DEV_CNTL_REFRESH) {
		x_dirty(x, -1);
	}
	else if(cmd == X_DCNTL_GET_INFO) {
		uint32_t i = 0;
		if(in != NULL)
			i = proto_read_int(in);

		x_display_t* display = &x->displays[i]; //TODO
		xscreen_t scr;	
		scr.id = 0;
		scr.fps = x->config.fps;
		scr.size.w = display->g->w;
		scr.size.h = display->g->h;
		PF->add(ret, &scr, sizeof(xscreen_t));
	}
	else if(cmd == X_DCNTL_GET_DISP_NUM) {
		PF->addi(ret, x->display_num);
	}
	else if(cmd == X_DCNTL_SET_XWM) {
		x->xwm_pid = from_pid;
		x_dirty(x, -1);
	}
	else if(cmd == X_DCNTL_UNSET_XWM) {
		x->xwm_pid = -1;
		x_dirty(x, -1);
	}
	else if(cmd == X_DCNTL_INPUT) {
		xevent_t ev;
		proto_read_to(in, &ev, sizeof(xevent_t));
		handle_input(x, from_pid, &ev);
	}
	return 0;
}

static int xserver_win_close(int fd, int from_pid, fsinfo_t* info, void* p) {
	(void)info;
	(void)fd;
	x_t* x = (x_t*)p;
	xwin_t* win = x_get_win(x, fd, from_pid);
	if(win == NULL) {
		return -1;
	}
	int disp_index = win->xinfo->display_index;
	x_del_win(x, win);	

	x_dirty(x, disp_index);
	return 0;
}

int xserver_step(void* p) {
	x_t* x = (x_t*)p;
	ipc_disable();
	x_repaint_req(x, -1);
	for(uint32_t i=0; i<x->display_num; i++) {
		x_repaint(x, i);
	}
	ipc_enable();
	usleep(1000000/x->config.fps);
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/x";
	const char* display_man = argc > 2 ? argv[2]: "/dev/display";
	const int32_t display_index = argc > 3 ? atoi(argv[3]): -1;

	x_t x;
	if(x_init(&x, display_man, display_index) != 0)
		return -1;

	read_config(&x, "/etc/x/x.conf");
	cursor_init(&x.cursor);

	int pid = -1;
	if(x.config.xwm[0] != 0) {
		pid = fork();
		if(pid == 0) {
			exec(x.config.xwm);
		}
		proc_wait_ready(pid);
		x.xwm_pid = pid;
	}

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "xserver");
	dev.fcntl = xserver_fcntl;
	dev.close = xserver_win_close;
	dev.open = xserver_win_open;
	dev.dev_cntl = xserver_dev_cntl;
	dev.loop_step = xserver_step;
	dev.extra_data = &x;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	x_close(&x);
	return 0;
}
