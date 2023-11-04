#ifndef XWM_HH
#define XWM_HH

#include <x/xwm.h>
#include <graph/graph_ex.h>
#include <font/font.h>
#include <sconf/sconf.h>

namespace Ewok {

class XWM {
protected:
	xwm_t xwm;
	uint32_t titleH;
	uint32_t frameW;

	font_t font;
	uint32_t bgColor;
	uint32_t fgColor;
	uint32_t bgTopColor;
	uint32_t fgTopColor;
	uint32_t desktopFGColor;
	uint32_t desktopBGColor;

	virtual void getColor(uint32_t *fg, uint32_t* bg, bool top);
	virtual void getWinSpace(int style, grect_t* xr, grect_t* wsr);
	virtual void getClose(xinfo_t* info, grect_t* r);
	virtual void getMin(xinfo_t* info, grect_t* r);
	virtual void getMinSize(xinfo_t* info, int* w, int* h);
	virtual void getMax(xinfo_t* info, grect_t* r);
	virtual void getTitle(xinfo_t* info, grect_t* r);
	virtual void getResize(xinfo_t* info, grect_t* r);

	virtual void drawDesktop(graph_t* g);
	virtual void drawTitle(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	virtual void drawMax(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	virtual void drawMin(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	virtual void drawClose(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	virtual void drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top);
	virtual void drawFrame(graph_t* g, xinfo_t* info, bool top);
	virtual void drawDragFrame(graph_t* g, grect_t* r);
	virtual void loadConfig(sconf_t* sconf);
public:
	inline void __getWinSpace(int style, grect_t* xr, grect_t* wsr) {getWinSpace(style, xr, wsr);}
	inline void __getClose(xinfo_t* info, grect_t* r) { getClose(info, r); }
	inline void __getMin(xinfo_t* info, grect_t* r) { getMin(info, r); }
	inline void __getMinSize(xinfo_t* info, int* w, int* h) { getMinSize(info, w, h); }
	inline void __getMax(xinfo_t* info, grect_t* r) { getMax(info, r); }
	inline void __getTitle(xinfo_t* info, grect_t* r) { getTitle(info, r); }
	inline void __getResize(xinfo_t* info, grect_t* r) { getResize(info, r); }
	inline void __drawDesktop(graph_t* g) { drawDesktop(g); }
	inline void __drawTitle(graph_t* g, xinfo_t* info, grect_t* r, bool top) {drawTitle(g, info, r, top);}
	inline void __drawMax(graph_t* g, xinfo_t* info, grect_t* r, bool top) {drawMax(g, info, r, top);}
	inline void __drawMin(graph_t* g, xinfo_t* info, grect_t* r, bool top) {drawMin(g, info, r, top);}
	inline void __drawClose(graph_t* g, xinfo_t* info, grect_t* r, bool top) {drawClose(g, info, r, top);}
	inline void __drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top) {drawResize(g, info, r, top); }
	inline void __drawFrame(graph_t* g, xinfo_t* info, bool top) {drawFrame(g, info, top); }
	inline void __drawDragFrame(graph_t* g, grect_t* r) {drawDragFrame(g, r); }

	XWM(void);
	virtual ~XWM(void) { }

	void readConfig(const char* fname);
	void run(void);
};

}
#endif
