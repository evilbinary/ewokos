#ifndef WIDGET_WIN_HH
#define WIDGET_WIN_HH

#include <x++/XWin.h>
#include <Widget/RootWidget.h>

namespace Ewok {

class WidgetWin: public XWin {
	bool painting;
	static const uint32_t TIMER_MIN_FPS = 1;
protected:
	uint32_t timerFPS;
	uint32_t timerStep;
	RootWidget* root;
	uint32_t timerID;
	void onRepaint(graph_t* g);
	void onResize(void);
	void onEvent(xevent_t* ev);
	bool onClose();
	void onOpen();
public:
	WidgetWin(void);
	~WidgetWin(void);
	inline RootWidget* getRoot() { return root; }

	void setRoot(RootWidget* root);
	void setTimer(uint32_t fps);
	void timerTask();
};

}

#endif
