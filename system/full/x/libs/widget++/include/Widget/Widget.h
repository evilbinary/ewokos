#ifndef WIDGET_HH
#define WIDGET_HH

#include <x++/XWin.h>
#include <string.h>
#include <sys/klog.h>

namespace Ewok {

class Container;
class RootWidget;
class Widget {
	friend Container;
	Widget* next;
	Widget* prev;

	bool isContainer;
protected:
	uint32_t id;
	uint32_t fgColor;
	uint32_t bgColor;
	int32_t marginH;
	int32_t marginV;
	Widget* father;

	bool dirty;
	bool fixed;
	bool alpha;

	grect_t area;

	virtual void onResize() { }
	virtual void onMove() { }
	virtual bool onMouse(xevent_t* ev) {  return false; }
	virtual bool onKey(xevent_t* ev) {  return false; }

	virtual void repaint(graph_t* g);
	virtual void onRepaint(graph_t* g);
	virtual bool onEvent(xevent_t* ev);
public:
	Widget(void);
	virtual ~Widget() { }

	inline void setFGColor(uint32_t color) { fgColor = color; update(); }
	inline void setBGColor(uint32_t color) { bgColor = color; update(); }
	inline void setMarginH(int32_t v) { marginH = v; }
	inline void setMarginV(int32_t v) { marginV = v; }
	inline void setFixed(bool fixed) { this->fixed = fixed; }
	inline void setAlpha(bool alpha) { this->alpha = alpha; }
	inline bool isAlpha() { return alpha; }

	void update();
	RootWidget* getRoot(void);
	void fixedMinSize(void);
	void resizeTo(int w, int h);
	void resize(int dw, int dh);
	void moveTo(int x, int y);
	void move(int dx, int dy);
	void setArea(int x, int y, int w, int h);
	gpos_t getRootPos(int32_t x = 0, int32_t y = 0);
	gpos_t getScreenPos(int32_t x = 0, int32_t y = 0);
	gpos_t getInsidePos(int32_t screenX, int32_t screenY);
	grect_t getRootArea(bool margin = true);
	grect_t getScreenArea(bool margin = true);

	virtual gsize_t getMinSize(void);
};

}

#endif