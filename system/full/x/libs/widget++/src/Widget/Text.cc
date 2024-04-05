#include <Widget/Text.h>
#include <ewoksys/utf8unicode.h>

namespace Ewok {

void Text::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	font_t* ft = font;
	if(ft == NULL)
		ft = theme->getFont();
	
	if(ft == NULL)
		return;
	if(fontSize == 0)
		fontSize = theme->basic.fontSize;

	graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);

	int i, x=0, y=0;
	pageSize = 0; 
	for(i=offset; i<contentSize; i++) {
		uint16_t c = content[i];
		uint16_t w, h;

		font_char_size(c, ft, fontSize, &w, NULL);
		h = fontSize;
		if((x+w) >= r.w || c == '\r' || c == '\n') {
			x = 0;
			y += h;
			if(y >= r.h)
				break;
		}

		if(c != '\r' && c != '\n') {
			graph_draw_unicode_font(g, r.x+x, r.y+y, c, ft, fontSize, theme->basic.fgColor, &w, &h);
			x += w;
		}
	}
	pageSize = i-offset;
	updateScroller();
}

Text::Text() {
	content = NULL;
	contentSize = 0;
	offset = 0;
	pageSize = 0;
	font = NULL;
	fontSize = 0;
}

Text::~Text() {
	if(content != NULL)
		free(content);
}

bool Text::onMouse(xevent_t* ev) {
	gpos_t ipos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
	if(ev->state == XEVT_MOUSE_DOWN) {
		last_mouse_down = ipos.y;
		update();
	}
	else if(ev->state == XEVT_MOUSE_DRAG) {
		int pos = ipos.y;
		int mv = (pos - last_mouse_down) * 2 / 16;
		if(mv != 0) {
			last_mouse_down = pos;
			scroll(mv, false);
		}
	}
	else if(ev->state == XEVT_MOUSE_MOVE) {
		if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_UP) {
			scroll(-1, false);
		}
		else if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_DOWN) {
			scroll(1, false);
		}
	}
	return true;
}

void Text::updateScroller() {
	setScrollerInfo(contentSize, offset, pageSize, false);
}

bool Text::onScroll(int step, bool horizontal) {
	int old_off = offset;
	//back to prev line start
	if(step > 0) {
		offset--;
		while(offset >= 0) {
			if(content[offset] == '\r' || content[offset] == '\n')
				break;
			offset--;
		}
	}
	else {
		offset++;
		while(offset < contentSize) {
			if(content[offset] == '\r' || content[offset] == '\n')
				break;
			offset++;
		}
	}

	if(offset < 0)
		offset = 0;
	else if((offset + pageSize) >= contentSize)
		offset = contentSize-pageSize;

	if(offset == old_off)
		return false;
	return true;
}

void Text::setContent(const char* ctnt, uint32_t size) {
	if(content != NULL) {
		free(content);
		content = NULL;
		contentSize = 0;
		offset = 0;
	}

	if(ctnt == NULL || ctnt[0] == 0 || size == 0) {
		updateScroller();
		update();
		return;
	}

	content = (uint16_t*)malloc((size+1)*2);
	contentSize = utf82unicode((uint8_t*)ctnt, size, content);
	updateScroller();
	update();
}

void Text::setFont(const string& name) {
	if(font != NULL)
		font_free(font);
	font = font_new(name.c_str(), true);
	update();
}

void Text::setFontSize(uint32_t size) {
	if(size < 12)
		size = 12;
	fontSize = size;
	update();
}

}