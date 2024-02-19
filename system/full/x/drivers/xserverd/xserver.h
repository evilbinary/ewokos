#ifndef XSERVER_H
#define XSERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <fb/fb.h>
#include <x/xcntl.h>
#include <x/xevent.h>
#include <x/xwm.h>
#include <display/display.h>
#include "cursor.h"
#include "xevtpool.h"

#define X_EVENT_MAX 16

enum {
	X_win_DRAG_MOVE = 1,
	X_win_DRAG_RESIZE
};

typedef struct st_xwin {
	int fd;
	int from_pid;
	uint32_t from_pid_uuid;
	graph_t* g;
	graph_t* g_buf;
	xinfo_t* xinfo;
	bool dirty;
	bool dirty_mark;

	grect_t r_title;
	grect_t r_close;
	grect_t r_min;
	grect_t r_max;
	grect_t r_resize;

	struct st_xwin *next;
	struct st_xwin *prev;
} xwin_t;

typedef struct {
	xwin_t* win_drag; //moving or resizing;
	gpos_t old_pos;
	gpos_t pos_delta;
	uint32_t drag_state;
} x_current_t;

typedef struct {
	uint32_t fps;
	bool bg_run;
	bool force_fullscreen;
} x_conf_t;

typedef struct {
	fb_t fb;
	graph_t* g;
	int32_t  g_shm_id;
	graph_t* g_fb;
	grect_t desktop_rect;
	bool dirty;
	bool cursor_task;
	bool need_repaint;
} x_display_t;

typedef struct {
	gpos_t down_pos;
	gpos_t last_pos;
	uint32_t state; 
} x_mouse_state_t;

typedef struct {
	const char* display_man;
	x_display_t displays[DISP_MAX];
	uint32_t display_num;
	uint32_t current_display;

	int xwm_pid;
	uint32_t xwm_uuid;

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

#endif