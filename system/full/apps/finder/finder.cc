#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sconf/sconf.h>
#include <upng/upng.h>
#include <x++/X.h>
#include <ewoksys/keydef.h>
#include <dirent.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/proc.h>
#include <ewoksys/klog.h>
#include <elf/elf.h>

using namespace Ewok;

class Finder: public XWin {
	uint32_t hideColor;
	uint32_t selectColor;
	uint32_t titleColor;
	uint32_t titleBGColor;
	graph_t* dirIcon;
	graph_t* fileIcon;
	graph_t* devIcon;

	sconf_t* fileTypes;
	int itemSize;

	int     mouse_last_y;
	int     selected;
	int     start;
	static const int MAX_FILES = 256;

	char cwd[FS_FULL_NAME_MAX+1];
	struct dirent files[MAX_FILES];
	int nums;

	void upBack(void) {
		if(strcmp(cwd, "/") == 0)
			return;

		int len = strlen(cwd)  - 1;
		for(int i=len; i>=0; i--) {
			if(cwd[i] == '/') {
				cwd[i] = 0;
				break;
			}
		}
		if(cwd[0] == 0)
			strcpy(cwd, "/");

		readDir(cwd);
		repaint();
	}

	bool check(const char* fname, const char* ext) {
		int i = strlen(fname) - strlen(ext);
		if(strcmp((fname+i), ext) == 0)
			return true;
		return false;
	}

	const char* fileType(const char* fname) {
		if(fileTypes == NULL)
			return "";
		for(int i=0; i<fileTypes->num;i++) {
			sconf_item_t* it = sconf_get_at(fileTypes, i);
			if(it == NULL || it->name->cstr[0] == 0)
				break;
			if(check(fname, it->name->cstr))
				return it->value->cstr;
		}
		return "";
	}

	bool check_elf(const char* fname) {
		elf_header_t header;
		if(elf_read_header(fname, &header) != 0)
			return false;

		int pid = fork();
		if(pid == 0)  {
			proc_detach();
			proc_exec(fname);
			exit(0);
		}
		return true;
	}

	void load(const char* fname) {
		if(check_elf(fname))
			return;

		char cmd[FS_FULL_NAME_MAX+1] = "";
		const char* prog = fileType(fname);

		if(prog[0] == 0)
			return;

		snprintf(cmd, FS_FULL_NAME_MAX, "%s %s", prog, fname);
		int pid = fork();
		if(pid == 0)  {
			proc_detach();
			proc_exec(cmd);
			exit(0);
		}
	}

	void runProc(int i) {
		struct dirent* it = &files[i];
		char fname[FS_FULL_NAME_MAX+1];
		if(strcmp(cwd, "/") == 0)
			snprintf(fname, FS_FULL_NAME_MAX, "/%s", it->d_name);
		else
			snprintf(fname, FS_FULL_NAME_MAX, "%s/%s", cwd, it->d_name);

		if(it->d_type == DT_DIR) {
			readDir(fname);
			repaint();
			return;
		}
		else 
			load(fname);
	}

	void readDir(const char* r) {
		DIR* dirp = opendir(r);
		if(dirp == NULL)
			return;
		if(r != cwd)	
			strcpy(cwd, r);
		selected = 0;
		nums = 0;
		start = 0;

		int i;
		for(i=0; i<MAX_FILES; i++) {
			struct dirent* it = readdir(dirp);
			if(it == NULL)
				break;
			memcpy(&files[i], it, sizeof(struct dirent));
		}
		closedir(dirp);
		nums = i;
	}

protected:
	void onRepaint(graph_t* g) {
		char name[FS_FULL_NAME_MAX+1];
		int h = itemSize;
		int yMargin = (itemSize - theme.getFont()->max_size.y)/2;
		int xMargin = 8;

		graph_clear(g, theme.basic.bgColor);

		graph_fill_3d(g, 0, 0, g->w, h, titleBGColor, false);

		if(strcmp(cwd, "/") == 0)
			snprintf(name, FS_FULL_NAME_MAX, "[%s]", cwd);
		else
			snprintf(name, FS_FULL_NAME_MAX, ".. [%s]", cwd);
		if(dirIcon != NULL) {
			int iconMargin = (itemSize - dirIcon->h)/2;
			graph_blt_alpha(dirIcon, 0, 0, dirIcon->w, dirIcon->h,
					g, xMargin, iconMargin, dirIcon->w, dirIcon->h, 0xff);
			xMargin += dirIcon->w + 4;
		}
		graph_draw_text_font(g, xMargin, yMargin, name, theme.getFont(), titleColor);

		for(int i=start; i<nums; i++) {
			struct dirent* it = &files[i];
			if(i == selected)
				graph_fill(g, 0, (i+1-start)*h, g->w, h, selectColor);

			uint32_t color = theme.basic.fgColor;
			if(it->d_name[0] == '.')
				color = hideColor;

			graph_t* icon = NULL;
			if(it->d_type == DT_DIR)
				icon = dirIcon;
			else if(it->d_type == DT_REG || it->d_type == DT_BLK)
				icon = fileIcon;
			else
				icon = devIcon;

			xMargin = 8;
			if(icon != NULL) {
				int iconMargin = (itemSize - dirIcon->h)/2;
				graph_blt_alpha(icon, 0, 0, icon->w, icon->h,
						g, xMargin, (i+1-start)*h+iconMargin, icon->w, icon->h, 0xff);
				xMargin += icon->w + 4;
			}
			graph_draw_text_font(g, xMargin, (i+1-start)*h+yMargin, it->d_name, theme.getFont(), color);
		}
	}

	void mouseHandle(xevent_t* ev) {
		gpos_t pos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
		int h = itemSize;
		if(ev->state == XEVT_MOUSE_DOWN) {
			mouse_last_y = ev->value.mouse.y;
			int at = pos.y / itemSize;
			selected = at-1 + start;
			repaint();
			return;
		}
		else if(ev->state == XEVT_MOUSE_DRAG) {
			int mv = (ev->value.mouse.y - mouse_last_y)/ h;
			if(abs_32(mv) > 0) {
				mouse_last_y = ev->value.mouse.y;
				//drag release
				start -= mv;
				if(start < 0)
					start = 0;
				else if(start >= nums)
					start = nums - 1;
				repaint();
			}
		}
		else if(ev->state == XEVT_MOUSE_CLICK) {
			int at = pos.y / itemSize;
			if(at == 0) {
				upBack();
				return;
			}
			at = (at-1) + start;
			if(at < nums) 
				runProc(at);
			return;
		}
	}

	void imHandle(xevent_t* ev) {
		xinfo_t xinfo;
		getInfo(xinfo);
		int h = itemSize;
		int lines = xinfo.wsr.h/h - 1;

		int key = ev->value.im.value;
		if(ev->state == XIM_STATE_PRESS) {
			if(key == KEY_UP)
				selected--;
			else if(key == KEY_DOWN)
				selected++;
			else
				return;
		}
		else {//RELEASE
			if(key == KEY_LEFT || key == KEY_BUTTON_B) {
				upBack();
				return;
			}
			else if(key == KEY_RIGHT || key == KEY_ENTER || key == KEY_BUTTON_A) {
				runProc(selected);
				return;
			}
		}

		if(selected >= nums)
			selected = nums-1;
		if(selected < 0)
			selected = 0;

		if(selected < start) {
			start -= lines;
			if(start < 0)
				start = 0;
		}
		else if((selected - start) >= lines) 
			start += lines - 1;
		repaint();
	}

	void onEvent(xevent_t* ev) {
		if(ev->type == XEVT_MOUSE) {
			mouseHandle(ev);
			return;	
		}
		else if(ev->type == XEVT_IM) {
			imHandle(ev);	
		}
	}

public:
	inline Finder() {
		hideColor = 0xff888888;
		selectColor = 0xff444444;
		titleColor = 0xffffff00;
		titleBGColor = 0xffaaaaaa;
		fileIcon = png_image_new(x_get_theme_fname(X_THEME_ROOT, "finder", "icons/file.png"));
		dirIcon = png_image_new(x_get_theme_fname(X_THEME_ROOT, "finder", "icons/folder.png"));
		devIcon = png_image_new(x_get_theme_fname(X_THEME_ROOT, "finder", "icons/device.png"));
		itemSize = 36;

		selected = 0;
		start = 0;
		mouse_last_y = 0;
		fileTypes = sconf_load("/usr/system/filetypes.conf");
		readDir("/");
	}

	inline ~Finder() {
		if(fileIcon != NULL)
			graph_free(fileIcon);
		if(dirIcon != NULL)
			graph_free(dirIcon);
		if(devIcon != NULL)
			graph_free(devIcon);
		if(fileTypes != NULL)
			sconf_free(fileTypes);
	}

	bool readConfig(const char* fname) {
		sconf_t *conf = sconf_load(fname);	
		if(conf == NULL)
			return false;

		theme.loadConfig(conf);

		const char* v = sconf_get(conf, "item_size");
		if(v[0] != 0)
			itemSize = atoi(v);
		if(theme.getFont()->max_size.y > itemSize)
			itemSize = theme.getFont()->max_size.y;

		v = sconf_get(conf, "select_color");
		if(v[0] != 0)
			selectColor = strtoul(v,NULL, 16);

		v = sconf_get(conf, "hide_color");
		if(v[0] != 0)
			hideColor = strtoul(v, NULL,16);

		v = sconf_get(conf, "title_color");
		if(v[0] != 0)
			titleColor = strtoul(v,NULL, 16);

		v = sconf_get(conf, "title_bg_color");
		if(v[0] != 0)
			titleBGColor = strtoul(v, NULL,16);

		v = sconf_get(conf, "file_icon");
		if(v[0] != 0) {
			graph_free(fileIcon);
			fileIcon = png_image_new(v);
		}

		v = sconf_get(conf, "dir_icon");
		if(v[0] != 0) {
			graph_free(dirIcon);
			dirIcon = png_image_new(v);
		}

		sconf_free(conf);
		return true;
	}
};

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	X x;
	Finder xwin;
	xwin.readConfig(x_get_theme_fname(X_THEME_ROOT, "finder", "theme.conf"));

	x.open(0, &xwin, 300, 0, "finder", XWIN_STYLE_NORMAL);

	xwin.setVisible(true);
	x.run(NULL);
	return 0;
}
