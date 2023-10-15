#ifndef XCNTL_H
#define XCNTL_H

#include <sys/ewokdef.h>
#include <graph/graph.h>

#ifdef __cplusplus
extern "C" {
#endif


enum {
	X_CNTL_NONE = 0,
	X_CNTL_NEW,
	X_CNTL_UPDATE,
	X_CNTL_UPDATE_INFO,
	X_CNTL_WIN_SPACE,
	X_CNTL_CALL_XIM
};

enum {
	X_DCNTL_GET_INFO = 0,
	X_DCNTL_GET_DISP_NUM,
	X_DCNTL_SET_XWM,
	X_DCNTL_UNSET_XWM,
	X_DCNTL_INPUT,
	X_DCNTL_GET_EVT,
	X_DCNTL_QUIT,
	X_DCNTL_GET_THEME
};

enum {
	X_STATE_NORMAL = 0,
	X_STATE_MAX,
	X_STATE_MIN,
	X_STATE_FULL_SCREEN
};

enum {
	X_CMD_PUSH_EVENT = 0
};

#define X_UPDATE_REBUILD 0x1
#define X_UPDATE_REFRESH 0x2

#define X_STYLE_NORMAL    0x0
#define X_STYLE_NO_FRAME  0x1
#define X_STYLE_NO_TITLE  0x2
#define X_STYLE_NO_RESIZE 0x4
#define X_STYLE_LAZY      0x8 //ignore all event
#define X_STYLE_NO_FOCUS  0x10
#define X_STYLE_SYSTOP    0x20
#define X_STYLE_SYSBOTTOM 0x40
#define X_STYLE_XIM       0x80
#define X_STYLE_LAUNCHER  0x100
#define X_STYLE_ANTI_FSCR 0x200 //anti full screen

#define X_TITLE_MAX 32
typedef struct {
	uint32_t win;
	void*    g_shm;
	uint32_t display_index;
	uint32_t style;
	uint32_t state;
	bool visible;
	bool repaint_lazy;
	bool alpha;
	grect_t wsr; //workspace rect
	grect_t winr; //window rect
	char title[X_TITLE_MAX];
} xinfo_t;

typedef struct {
	int id;
	int fps;
	gsize_t size;
} xscreen_t;

#ifdef __cplusplus
}
#endif

#endif
