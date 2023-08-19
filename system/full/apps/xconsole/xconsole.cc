#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <console/console.h>
#include <sconf/sconf.h>
#include <sys/vfs.h>
#include <sys/keydef.h>
#include <sys/klog.h>
#include <ttf/ttf.h>
#include <x++/X.h>

using namespace Ewok;

typedef struct {
	uint32_t fg_color;
	uint32_t bg_color;
	uint32_t unfocus_fg_color;
	uint32_t unfocus_bg_color;
	uint32_t buffer_rows;
} conf_t;

class XConsole : public XWin {
	conf_t conf;
	console_t console;
	int32_t rollStepRows;
public:
	XConsole() {
		console_init(&console);
	}

	~XConsole() {
		console_close(&console);
	}

	bool readConfig(const char* fname) {
		memset(&conf, 0, sizeof(conf_t));
		sconf_t *sconf = sconf_load(fname);	
		if(sconf == NULL)
			return false;

		const char* v = sconf_get(sconf, "bg_color");
		if(v[0] != 0) 
			conf.bg_color = atoi_base(v, 16);

		v = sconf_get(sconf, "fg_color");
		if(v[0] != 0) 
			conf.fg_color = atoi_base(v, 16);

		v = sconf_get(sconf, "unfocus_fg_color");
		if(v[0] != 0) 
			conf.unfocus_fg_color = atoi_base(v, 16);

		v = sconf_get(sconf, "unfocus_bg_color");
		if(v[0] != 0) 
			conf.unfocus_bg_color = atoi_base(v, 16);

		v = sconf_get(sconf, "buffer_rows");
		if(v[0] != 0) 
			conf.buffer_rows = atoi(v);

		uint32_t font_size = 16;
		v = sconf_get(sconf, "font_size");
		if(v[0] != 0) 
			font_size = atoi(v);

		v = sconf_get(sconf, "font");
		if(v[0] == 0) 
			v = "/data/fonts/system.ttf";
		
		font_load(v, font_size, &console.font);

		v = sconf_get(sconf, "font_margin");
		if(v[0] != 0) 
			console.font_margin = atoi(v);

		sconf_free(sconf);

		console.fg_color = conf.fg_color;
		console.bg_color = conf.bg_color;
		return true;
	}

	void put(const char* buf, int size) {
		console_put_string(&console, buf, size);
	}

	void rollEnd(void) {
		console_roll(&console, 0x0fffffff); //roll forward to the last row
	}

protected:
	void onFocus(void) {
		console.fg_color = conf.fg_color;
		console.bg_color = conf.bg_color;
		repaint();
		callXIM();
	}

	void onUnfocus(void) {
		console.fg_color = conf.unfocus_fg_color;
		console.bg_color = conf.unfocus_bg_color;
		repaint();
	}

	void onRepaint(graph_t* g) {
		if(console.w != g->w || console.h != g->h) {
			uint32_t buffer_rows = 0;
			if(console.font.id >=0) {
				buffer_rows = (g->h / console.font.max_size.y)*4;
				rollStepRows = (g->h / console.font.max_size.y) / 2;
			}
			if(conf.buffer_rows > buffer_rows) {
				buffer_rows = conf.buffer_rows;
				rollStepRows = 8;
			}
			console_reset(&console, g->w, g->h, buffer_rows);
		}
		console_refresh(&console, g);
	}

	void onEvent(xevent_t* ev) {
		if(ev->type == XEVT_IM && ev->state == XIM_STATE_PRESS) {
			int c = ev->value.im.value;
			if(c == KEY_ROLL_BACK) {
				console_roll(&console, -(rollStepRows));
				repaint();
				return;
			}
			else if(c == KEY_ROLL_FORWARD) {
				console_roll(&console, (rollStepRows));
				repaint();
				return;
			}

			if(c != 0) {
				write(1, &c, 1);
			}
		}
	}
};

static void loop(void* p) {
	XConsole* console = (XConsole*)p;

	char buf[512];
	int size = read(0, buf, 512);
	if(size > 0) {
		console->put(buf, size);
		console->rollEnd();
		console->repaint();
		return;
	}
	if(errno != EAGAIN) 
		console->close();
	else
		usleep(3000);
}

static int run(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	int flags = fcntl(0, F_GETFL, 0);
	fcntl(0, F_SETFL, flags | O_NONBLOCK);
	flags = fcntl(1, F_GETFL, 0);
	fcntl(1, F_SETFL, flags | O_NONBLOCK);

	XConsole xwin;
	xwin.readConfig("/etc/x/xconsole.conf");

	X x;
	xscreen_t scr;
 	x.screenInfo(scr, 0);
	x.open(&xwin, 64, 40, scr.size.w*3/4, scr.size.h*3/4, "xconsole", 0);
	xwin.setVisible(true);

	x.run(loop, &xwin);
	return 0;
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	int fds1[2];
	int fds2[2];
	pipe(fds1);
	pipe(fds2);

	int pid = fork();
	if(pid != 0) { //father proc for p2 reader.
		dup2(fds1[0], 0);
		dup2(fds2[1], 1);
		close(fds1[0]);
		close(fds1[1]);
		close(fds2[0]);
		close(fds2[1]);
		return run(argc, argv);
	}
	//child proc for p1 writer
	dup2(fds1[1], 1);
	dup2(fds2[0], 0);
	close(fds1[0]);
	close(fds1[1]);
	close(fds2[0]);
	close(fds2[1]);
	setenv("CONSOLE", "xconsole");
	setenv("CONSOLE_ID", "console-x");

	return exec("/bin/shell");
}
