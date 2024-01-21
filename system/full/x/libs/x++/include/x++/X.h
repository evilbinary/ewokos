#ifndef X_HH
#define X_HH

#include <x/xwin.h>
#include <graph/graph.h>
#include <x++/XWin.h>
#include <ewoksys/vfs.h>
#include <stdio.h>
#include <font/font.h>

namespace Ewok {

class X {
	x_t x;
public:
	inline x_t* c_x(void) { return &x; }
	X(void);
	void run(void (*loop)(void*), void* p = NULL);
	void terminate(void);
	bool open(uint32_t dispIndex, XWin* xwin, int x, int y, uint32_t w, uint32_t h, const char* title, uint32_t style);
	bool open(uint32_t dispIndex, XWin* xwin, uint32_t w, uint32_t h, const char* title, uint32_t style);

	static const char* getResName(const char* name);

	static uint32_t getDisplayNum();
	static bool    getScreenInfo(xscreen_t& scr, int index = 0);
	static bool    getDesktopSpace(grect_t& r, int index = 0);
	static bool    setDesktopSpace(const grect_t& r, int index = 0);
	static font_t* getSysFont(void);
};

}
#endif
