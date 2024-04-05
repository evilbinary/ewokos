#ifndef WIDGET_LABEL_HH
#define WIDGET_LABEL_HH

#include <Widget/Widget.h>
#include <string>

using namespace EwokSTL;
namespace Ewok {

class Label: public Widget {
protected:
	string label;
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);

public:
	Label(const string& str);
	~Label(void);

	void setLabel(const string& str);
	const string& getLabel() { return label; }
};

}

#endif
