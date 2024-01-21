#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <vprintf.h>
#include <upng/upng.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/kernel_tic.h>
#include <font/font.h>
#include <x++/X.h>
#include <ewoksys/timer.h>
#include <ewoksys/klog.h>

using namespace Ewok;

class PowerInfoX : public XWin {
	font_t font;
	int powerFD;

	void drawCharging(graph_t* g, const grect_t& r, int bat) {
		static int b = 0;
		int w = r.w*bat*b/300;
		graph_gradation(g, r.x+r.w-w, r.y, w, r.h, 0xffffffff, 0xff22dd22, true);

		char txt[8];
		snprintf(txt, 7, "%d%%", bat);
		graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
					txt, &font, 0xff000000, FONT_ALIGN_CENTER);
		b++;
		b%=4;
	}

	void drawBat(graph_t* g, const grect_t& r, int bat) {
		int w = r.w*bat/100;
		graph_gradation(g, r.x+r.w-w, r.y, w, r.h, 0xffffffff, 0xff22dd22, true);

		char txt[8];
		snprintf(txt, 7, "%d%%", bat);
		graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
					txt, &font, 0xff000000, FONT_ALIGN_CENTER);
	}

	void drawBase(graph_t* g, grect_t& r) {
		graph_gradation(g, r.x, r.y+4, 5, r.h-8, 0xffffffff, 0xffaaaaaa, true);
		graph_box(g, r.x, r.y+4, 5, r.h-8, 0xff000000);
		r.x += 4;
		r.w -= 4;

		graph_gradation(g, r.x, r.y, r.w, r.h, 0xffffffff, 0xff888888, true);
		graph_box(g, r.x, r.y, r.w, r.h, 0xff000000);
		r.x++;
		r.y++;
		r.w -= 2;
		r.h -= 2;
	}

protected:
	void onEvent(xevent_t* ev) {
		if(ev->type == XEVT_MOUSE) {
			if(ev->state == XEVT_MOUSE_CLICK)
				this->close();
			else if(ev->state == XEVT_MOUSE_DRAG)
				this->move(ev->value.mouse.rx, ev->value.mouse.ry);
		}
	}

	void onRepaint(graph_t* g) {
		setAlpha(true);
		graph_clear(g, 0x0);
		grect_t r = {4, 4, g->w-8, g->h-8};
		drawBase(g, r);

		if(powerFD < 0)
			powerFD = open("/dev/power0", O_RDONLY);

		if(powerFD < 0)
			return;

		uint8_t buf[4];
		if(read(powerFD, buf, 3) != 3)
			return;
			
		if(buf[0] == 0)
			return;

		if(buf[1])
			drawCharging(g, r, buf[2]);
		else
			drawBat(g, r, buf[2]);
	}

public:
	inline PowerInfoX() {
		powerFD = -1;
		font_load(DEFAULT_SYSTEM_FONT, 13, &font, true);
	}
	
	inline ~PowerInfoX() {
		if(font.id >= 0)
			font_close(&font);
		if(powerFD > 0)
			::close(powerFD);
	}
};

static XWin* _xwin = NULL;
static void timer_handler(void) {
	_xwin->repaint();
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	X x;
	PowerInfoX xwin;
	x.open(0, &xwin, 86, 42, "pwrInfo", XWIN_STYLE_NO_FRAME | XWIN_STYLE_ANTI_FSCR);
	xwin.setVisible(true);

	_xwin = &xwin;
	uint32_t tid = timer_set(1000000, timer_handler);
	x.run(NULL, &xwin);
	timer_remove(tid);

	return 0;
} 
