#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <ewoksys/keydef.h>
#include <ewoksys/session.h>
#include <ewoksys/klog.h>
#include <ewoksys/proc.h>
#include <upng/upng.h>
#include <x++/X.h>

using namespace Ewok;

#ifdef __cplusplus
extern "C" { extern int setenv(const char*, const char*);}
#endif

static void run_xinit(session_info_t* info) {
	vfs_create(info->home, NULL, FS_TYPE_DIR, 0750, false, true);
	chown(info->home, info->uid, info->gid);

	setgid(info->gid);
	setuid(info->uid);
	setenv("HOME", info->home);

	proc_exec("/bin/shell /etc/x/xinit.rd");
}

class XSession : public XWin {
	EwokSTL::string username;
	EwokSTL::string password;
	EwokSTL::string errMsg;
	graph_t *logoUser, *logoPasswd;
	bool passwordMode;

	int login() {
		session_info_t info;
		int res = session_check(username.c_str(), password.c_str(), &info);
		if(res != 0)
			return res;

		close();
		run_xinit(&info);
		return 0;
	}

	void drawBG(graph_t* g) {
		graph_draw_dot_pattern(g, 0, 0, g->w, g->h,
				theme.basic.bgColor, theme.basic.fgColor, 1);
	}

	void drawFrame(graph_t* g, const grect_t& r) {
		font_t* font = theme.getFont();
		graph_fill(g, r.x, r.y, r.w, r.h, theme.basic.bgColor);
		graph_fill(g, r.x, r.y, r.w, theme.basic.fontSize, 0xffffbb88);
		graph_draw_text_font(g, r.x+8, r.y,
				"EwokOS(M-kernel)", font, theme.basic.fontSize, theme.basic.fgColor);

		graph_t* logo = logoUser;
		if(passwordMode)
			logo = logoPasswd;
		graph_blt_alpha(logo, 0, 0, logo->w, logo->h,
				g, r.x+r.w-logo->w, r.y, logo->w, logo->h, 0xff);
		graph_box_3d(g, r.x, r.y, r.w, r.h, theme.basic.bgColor, false);
	}

	void drawInput(graph_t* g, const grect_t& r, const char* title, const char* input) {
		font_t* font = theme.getFont();

		int y = r.y + theme.basic.fontSize+8;
		graph_draw_text_font(g, r.x+8, y, 
				title, font, theme.basic.fontSize , theme.basic.fgColor);

		y += theme.basic.fontSize+8;
		graph_fill_3d(g, r.x+8, y, r.w-16, theme.basic.fontSize, theme.basic.bgColor, true);
		if(passwordMode) {
			EwokSTL::string pwd;
			int len = strlen(input);
			if(len > 0) {
				int i;
				for(i=0; i<(len-1); i++)
					pwd += '*';
				pwd += input[i];
				graph_draw_text_font_align(g, r.x, y, r.w, theme.basic.fontSize,
						pwd.c_str(), font, theme.basic.fontSize, theme.basic.fgColor, FONT_ALIGN_CENTER);
			}
		}
		else
			graph_draw_text_font_align(g, r.x, y, r.w, theme.basic.fontSize,
					input, font, theme.basic.fontSize, theme.basic.fgColor, FONT_ALIGN_CENTER);

		if(errMsg.length() != 0) {
			y += theme.basic.fontSize;
			graph_draw_text_font_align(g, r.x, y, r.w, theme.basic.fontSize, 
					errMsg.c_str(), font, theme.basic.fontSize, 0xffff0000, FONT_ALIGN_CENTER);
		}
	}

protected:
	void onRepaint(graph_t* g) {
		font_t* font = theme.getFont();
		const char* input = passwordMode ? password.c_str() : username.c_str();
		const char* title = passwordMode ? "password" : "username";

		uint32_t tw, th, iw, ih;
		font_text_size(title, font, theme.basic.fontSize, &tw, &th);
		font_text_size(input, font, theme.basic.fontSize, &iw, &ih);
		if(iw < 180)
			iw = 180;

		uint32_t fw = iw+16;
		uint32_t fh = ih * 5;
		grect_t frameR = { (g->w - fw) / 2, (g->h - fh) / 2, fw, fh };

		drawBG(g);
		drawFrame(g, frameR);
		drawInput(g, frameR, title, input);
	}

	void onEvent(xevent_t* ev) {
		if(ev->type == XEVT_IM && ev->state == XIM_STATE_PRESS) {
			EwokSTL::string &input = passwordMode ? password:username;
			int c = ev->value.im.value;
			int len = input.length();

			if(c == KEY_BACKSPACE || c == CONSOLE_LEFT) {
				if(len > 0)
					input = input.substr(0, len-1);
				errMsg = "";
			}
			else if(c == KEY_ENTER) {
				if(!passwordMode) {
					if(username.length() == 0)
						return;

					int res = login();
					if(res == SESSION_ERR_PWD) {
						passwordMode = true;
						errMsg = "";
					}
					else if(res == SESSION_ERR_USR) {
						password = "";
						errMsg = "User not existed!";
						passwordMode = false;
					}
				}
				else {
					if(login() != 0) {
						username = "";
						password = "";
						errMsg = "Wrong password!";
						passwordMode = false;
					}
				}
			}
			else if(c > 20) {
				if(len < 16)
					input += c;
				errMsg = "";
			}
			repaint();
		}
	}

public:
	XSession() {
		theme.setFont(theme.basic.fontName, 16);
		passwordMode = false;
		logoUser = png_image_new("/usr/system/icons/ewok.png");
		logoPasswd = png_image_new("/usr/system/icons/storm.png");
	}

	~XSession() {
		if(logoUser != NULL)
			graph_free(logoUser);
		if(logoPasswd != NULL)
			graph_free(logoPasswd);
	}
};

int main(int argc, char* argv[]) {
	if(argc > 1) {
		//no login check
		session_info_t info;
		if(session_get_by_name(argv[1], &info) == 0) {
			run_xinit(&info);
			return 0;
		}
	}

	X x;
	XSession xwin;
	xwin.open(&x, 0, 0, 0, 0, 0, "XSessioin", XWIN_STYLE_NO_FRAME | XWIN_STYLE_SYSTOP);
	xwin.fullscreen();
	xwin.callXIM();

	x.run(NULL);
	return 0;
}

