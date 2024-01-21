#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/keydef.h>
#include <font/font.h>
#include <x++/X.h>

using namespace Ewok;

class XIMX : public XWin {
 	int xPid;

	static const int TYPE_KEYB = 0;
	static const int TYPE_OTHER = 1;
	const char* keytable[2];
	int keytableType;

	int kbFD;
	font_t font;
	gsize_t scrSize;
	gsize_t panelSize;
	bool hideMode;

	static const int INPUT_MAX = 128;
	char inputS[INPUT_MAX];

	int col, row, keyw, keyh, keySelect;
protected:
	int get_at(int x, int y) {
		if(!hideMode) {
			int input_h = font.max_size.y + 8;
			y -= input_h;
		}

		if(x > col * keyw) 
			return -1;
		if(y > row * keyh) 
			return -1;

		int i = (x / keyw);
		int j = (y / keyh);
		int at = i+j*col;
		if(at >= (int)strlen(keytable[keytableType]))
			return -1;
		return at;
	}

	void changeKeyTable(void) {
		if(keytableType == 0)
			keytableType = 1;
		else
			keytableType = 0;
		repaint();
	}

	void changeMode(bool hide) {
		hideMode = hide;
		if(hideMode) {
			int kw = 42;
			resizeTo(kw, kw);
			moveTo(scrSize.w - kw, scrSize.h - kw);
		}
		else { //show panel and repos
			int w = scrSize.w;
			int h = scrSize.h / 2;

			if(panelSize.w > 0)
				w = panelSize.w;
			if(panelSize.h > 0)
				h = panelSize.h;

			resizeTo(w, h);
			moveTo(scrSize.w-w, scrSize.h-h);
		}
	}

	void doKeyIn(char c) {
		if(c == '\1') {
			changeMode(true);
			return;
		}
		else if(c == '\2') {
			changeKeyTable();
			return;
		}
		else if(c == '\3')
			c = keytable[keytableType][keySelect-1];
		else if(c == '\b')
			c = KEY_BACKSPACE;
		input(c);
	}

	void doMouseEvent(xevent_t* ev) {
		gpos_t pos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
		int x = pos.x;
		int y = pos.y;
		int at = get_at(x, y);
		if(at < 0)
			return;

		if(ev->state == XEVT_MOUSE_DOWN) {
			keySelect = at;
			repaint();
		}
		else if(ev->state == XEVT_MOUSE_DRAG) {
			if(keySelect >= 0 && keySelect != at) {
				keySelect = at;
				repaint();
			}
		}
		else if(ev->state == XEVT_MOUSE_UP) {
			if(hideMode) {
				changeMode(false);
				keySelect = -1;
				return;
			}

			char c = keytable[keytableType][keySelect];
			doKeyIn(c);	
			keySelect = -1;
			repaint();
		}
	}

	void doIMEvent(xevent_t* ev) {
		uint8_t c = ev->value.im.value;
		int32_t keyNum = strlen(keytable[keytableType]);

		if(ev->state == XIM_STATE_PRESS) {
			if(c == KEY_LEFT) {
				keySelect--;
			}
			else if(c == KEY_RIGHT) {
				keySelect++;
			}
			else if(c == KEY_UP) {
				if(hideMode)
					doKeyIn(c);
				else if(keySelect > col)
					keySelect -= col;
			}
			else if(c == KEY_DOWN) {
				if(hideMode)
					doKeyIn(c);
				else if((keySelect+col) < keyNum)
					keySelect += col;
			}
			else
				return;
		}
		else { //RELEASE
			if(c == KEY_BUTTON_SELECT) {
				changeMode(!hideMode);
				return;
			}
			else if(c == KEY_BUTTON_Y) {
				doKeyIn('\b');
				repaint();
				return;
			}
			else if(c == KEY_BUTTON_START) {
				doKeyIn('\n');
				changeMode(true);
				return;
			}
			else if(c == KEY_BUTTON_X) {
				doKeyIn(KEY_ROLL_BACK);
				repaint();
				return;
			}
			else if(c == KEY_BUTTON_B) {
				doKeyIn(KEY_ROLL_FORWARD);
				repaint();
				return;
			}
			else if(c == KEY_ENTER || c == KEY_BUTTON_A) {
				if(hideMode) {
					changeMode(false);
					return;
				}
				else {
					c = keytable[keytableType][keySelect];
					doKeyIn(c);
					repaint();
					return;
				}
			}
		}

		if(keySelect < 0)
			keySelect = 0;
		else if(keySelect >= keyNum)
			keySelect = keyNum - 1;
		repaint();
	}

	void onEvent(xevent_t* ev) {
		if(keyw == 0 || keyh == 0)
			return;

		if(ev->type == XEVT_MOUSE) {
			doMouseEvent(ev);
		}
		else if(ev->type == XEVT_IM) {
			doIMEvent(ev);
		}
	}

	void draw_input(graph_t* g, int input_h) {
		uint32_t w;
		graph_fill(g, 0, 0, g->w, input_h, 0xffffffff);
		font_text_size(inputS, &font, &w, NULL);
		graph_draw_text_font(g, (g->w-w)/2, 2, inputS, &font, 0xff000000);
		graph_box(g, 0, 0, g->w, input_h, 0xff000000);
	}

	const char* getKeyTitle(char c) {
		static char s[16];
		s[0] = c;
		s[1] = 0;

		if(c == ' ')
			strcpy(s, "SPC");
		else if(c == '\n')
			strcpy(s, "ENT");
		else if(c == '\b')
			strcpy(s, "BK");
		else if(c == '\2')
			strcpy(s, "C/#");
		else if(c == '\1')
			strcpy(s, "|||");
		else if(c == '\4')
			strcpy(s, "RB");

		return s;
	}

	void onRepaint(graph_t* g) {
		uint32_t font_h = font.max_size.y;
		if(hideMode) {
			graph_clear(g, 0xffaaaaaa);
			graph_draw_text_font(g,  2, 
					(g->h - font_h)/2,
					"|||", &font, 0xff000000);
			graph_box(g, 0, 0, g->w, g->h, 0xffdddddd);
			return;
		}

		graph_fill(g, 0, 0, g->w, g->w, 0xffcccccc);

		int input_h = font_h + 8;
		keyh = (g->h - input_h) / row;
		keyw = g->w / col;

		draw_input(g, input_h);
		const char* t = "";
		for(int j=0; j<row; j++) {
			for(int i=0; i<col; i++) {
				int at = i+j*col;
				if(at >= (int)strlen(keytable[keytableType]))
					break;
				char c = keytable[keytableType][at];
				int kx, ky, kw, kh;
				kx = i * keyw;
				ky = j * keyh+input_h;
				kw = keyw;
				kh = keyh;

				if(c >= 'a' && c <= 'z') {
					c += ('A' - 'a');
					graph_fill(g, kx, ky, kw, keyh, 0xffeeeeee);
				}
				
				if(c == '\3') //two key size
					kx -= keyw;
				if(c == ' ' || c == '\n' || c == '\3') //two key size
					kw = keyw * 2;

				if(keySelect == at) { //hot key
					ky -= (j == 0 ? input_h : keyh/2);
					kh = keyh + (j == 0 ? input_h : keyh/2);
					graph_fill(g, kx, ky, kw, kh, 0xffffffff);
				}

				if(c != '\3')
					t = getKeyTitle(c);
				else if(keySelect != at)
					continue;

				uint32_t tw;
				font_text_size(t, &font, &tw, NULL);
				if(keySelect == at) //hot key
					graph_draw_text_font(g, kx + (kw-tw)/2, 
							ky + 2,
							t, &font, 0xff000000);
				else
					graph_draw_text_font(g, kx + (kw-tw)/2, 
							ky + (kh - font_h)/2,
							t, &font, 0xff000000);
				graph_box(g, kx, ky, kw, kh, 0xffaaaaaa);
			}
		}
	}

	void input(char c) {
		xevent_t ev;
		ev.type = XEVT_IM;
		ev.value.im.value = c;
		ev.state = XIM_STATE_PRESS;

		int len = strlen(inputS);
		if(c == KEY_BACKSPACE) {
			if(len > 0)
				inputS[len-1] = 0;
		}
		else if(c == '\n') {
			inputS[0] = 0;
		}
		else if(c == '\4') {
			ev.value.im.value = KEY_ROLL_BACK;
		}
		else if(c < 0xF0 && len < INPUT_MAX-1) {
			if(!hideMode) {
				inputS[len] = c;
				inputS[len+1] = 0;
			}	
		}

		proto_t in;
		PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
		dev_cntl_by_pid(xPid, X_DCNTL_INPUT, &in, NULL);
		PF->clear(&in);
	}

	void onFocus(void) {
		changeMode(false);
	}

public:
	inline XIMX(int fw, int fh, int pw, int ph) {
		scrSize.w = fw;
		scrSize.h = fh;
		panelSize.w = pw;
		panelSize.h = ph;
		font_load(DEFAULT_SYSTEM_FONT, 13, &font, true);
		keytable[1] = ""
			"1234567890%-+\b"
			"\\#$&*(){}[]!\n\3"
			"\2:;\"'<>. \3`?^\1";
		keytable[0] = ""
			"qwertyuiop-/|\b"
			"~asdfghjkl@_\n\3"
			"\2zxcvbnm \3\4,.\1";
		keytableType = 0;

		col = 14;
		row = 3;
		keyh = font.max_size.y + 12;
		keyw = font.max_size.x*2 + 12;
		xPid = dev_get_pid("/dev/x");
		keySelect = -1;
		hideMode = false;
		inputS[0] = 0;
	}

	inline ~XIMX() {
		if(kbFD > 0)
			::close(kbFD);
		::close(xPid);
		if(font.id >= 0)
			font_close(&font);
	}
};

int main(int argc, char* argv[]) {
	X x;
	xscreen_t scr;
	x.getScreenInfo(scr, 0);

	int pw = scr.size.w/2;
	int ph = scr.size.h/2;
	if(argc > 1)
		pw = atoi(argv[1]);
	if(argc > 2)
		ph = atoi(argv[2]);

	XIMX xwin(scr.size.w, scr.size.h, pw, ph);
	x.open(0, &xwin, scr.size.w - pw, scr.size.h - ph, pw, ph, "xim",
			XWIN_STYLE_NO_FRAME | XWIN_STYLE_NO_FOCUS | XWIN_STYLE_SYSTOP | XWIN_STYLE_XIM | XWIN_STYLE_ANTI_FSCR);
	x.run(NULL, &xwin);
	return 0;
}
