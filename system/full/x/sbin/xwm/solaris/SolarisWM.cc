#include "SolarisWM.h"
#include <sys/kernel_tic.h>
#include <sys/klog.h>
#include <upng/upng.h>
#include <sconf/sconf.h>
#include <stdlib.h>

using namespace Ewok;

void SolarisWM::drawDesktop(graph_t* g) {
	graph_clear(g, desktopBGColor);
	//background pattern
	int32_t x, y;
	for(y=10; y<(int32_t)g->h; y+=10) {
		for(x=0; x<(int32_t)g->w; x+=10) {
			graph_pixel(g, x, y, desktopFGColor);
		}
	}

	if(bgImg != NULL)
		graph_blt(bgImg, 0, 0, bgImg->w, bgImg->h,
				g, (g->w - bgImg->w)/2, (g->h - bgImg->h)/2, bgImg->w, bgImg->h);
}

void SolarisWM::getBorderColor(uint32_t bg, uint32_t *dark, uint32_t *bright) {
	uint32_t a, r, g, b, c;
	a = (bg >> 24) & 0xff;
	r = (bg >> 16) & 0xff;
	g = (bg >> 8) & 0xff;
	b = bg & 0xff;

	c = r<g ? r:g;
	c = c<b ? c:b;

	*dark = argb(a, (c/3)*2, (c/3)*2, (c/3)*2);

	r = r<0xAA ? r:0xAA;
	g = g<0xAA ? g:0xAA;
	b = b<0xAA ? b:0xAA;

	c = r>g ? r:g;
	c = c>b ? c:b;
	*bright = argb(a, (c/3)*4, (c/3)*4, (c/3)*4);
}

void SolarisWM::drawMin(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	uint32_t dark, bright;
	getBorderColor(bg, &dark, &bright);

	graph_fill(g, r->x, r->y, r->w, r->h, bg);
	graph_box_3d(g, r->x, r->y, r->w, r->h, bright, dark);
	graph_box_3d(g, r->x+(r->w/2)-3, r->y+(r->h/2)-3,
			6, 6, bright, dark);
}

void SolarisWM::drawMax(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	uint32_t dark, bright;
	getBorderColor(bg, &dark, &bright);

	graph_fill(g, r->x, r->y, r->w, r->h, bg);
	graph_box_3d(g, r->x, r->y, r->w, r->h, bright, dark);
	graph_box_3d(g, r->x+3, r->y+3,
			r->w-6, r->h-6, bright, dark);
}

void SolarisWM::drawClose(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	uint32_t dark, bright;
	getBorderColor(bg, &dark, &bright);

	graph_fill(g, r->x, r->y, r->w, r->h, bg);
	graph_box_3d(g, r->x, r->y, r->w, r->h, bright, dark);
	graph_box_3d(g, r->x+5, r->y+(r->h/2)-2,
			r->w-10, 4, bright, dark);
}

void SolarisWM::drawResize(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	(void)info;
	if(!top)
		return;
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	uint32_t dark, bright;
	getBorderColor(bg, &dark, &bright);

	graph_line(g, 
			r->x + r->w - frameW, r->y,
			r->x + r->w, r->y, dark);
	graph_line(g,
			r->x + r->w - frameW, r->y + 1,
			r->x + r->w, r->y + 1, bright);
	graph_line(g,
			r->x, r->y + r->h - frameW,
			r->x, r->y + r->h, dark);
	graph_line(g,
			r->x + 1, r->y + r->h - frameW,
			r->x + 1, r->y + r->h, bright);
}

void SolarisWM::drawFrame(graph_t* graph, xinfo_t* info, bool top) {
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	uint32_t dark, bright;
	getBorderColor(bg, &dark, &bright);

	int x = info->wsr.x;
	int y = info->wsr.y;
	int w = info->wsr.w;
	int h = info->wsr.h;

	if((info->style & X_STYLE_NO_TITLE) == 0) {
		h += titleH;
		y -= titleH;
	}


	//win box
	graph_box_3d(graph, x-frameW, y-frameW, w+frameW*2, h+frameW*2, bright, dark);
	for(uint32_t i=1; i<(frameW-1); i++) {
		graph_box(graph, x-(frameW-i), y-(frameW-i), w+(frameW-i)*2, h+(frameW-i)*2, bg);
	}
	graph_box_3d(graph, x-1, y-1, w+2, h+2, dark, bright);
	//shadow
	/*if(top) {
		graph_fill(graph, x+w+frameW, y, frameW, h+frameW, 0xaa000000);
		graph_fill(graph, x, y+h+frameW, w+frameW*2, frameW, 0xaa000000);
	}
	*/
}

void SolarisWM::drawTitle(graph_t* g, xinfo_t* info, grect_t* r, bool top) {
	uint32_t fg, bg;
	getColor(&fg, &bg, top);

	uint32_t dark, bright;
	getBorderColor(bg, &dark, &bright);

	gsize_t sz;
	font_text_size(info->title, &font, (uint32_t*)&sz.w, (uint32_t*)&sz.h);
	
	int pw = (r->w-sz.w)/2;
	graph_fill(g, r->x, r->y, r->w, r->h, bg);
	graph_draw_text_font(g, r->x+pw, r->y+2, info->title, &font, fg);//title
	graph_box_3d(g, r->x, r->y, r->w, r->h, bright, dark);
}

void SolarisWM::readConfig(void) {
	sconf_t *sconf = sconf_load("/etc/x/xwm_solaris.conf");	
	if(sconf == NULL)
		return;

	const char* v = sconf_get(sconf, "bg_image");
	if(v[0] != 0) 
		bgImg = png_image_new(v);

	v = sconf_get(sconf, "fg_color");
	if(v[0] != 0) 
		fgColor = atoi_base(v, 16);

	v = sconf_get(sconf, "bg_color");
	if(v[0] != 0) 
		bgColor = atoi_base(v, 16);

	v = sconf_get(sconf, "fg_top_color");
	if(v[0] != 0) 
		fgTopColor = atoi_base(v, 16);

	v = sconf_get(sconf, "bg_top_color");
	if(v[0] != 0) 
		bgTopColor = atoi_base(v, 16);

	v = sconf_get(sconf, "desktop_fg_color");
	if(v[0] != 0) 
		desktopFGColor = atoi_base(v, 16);

	v = sconf_get(sconf, "desktop_bg_color");
	if(v[0] != 0) 
		desktopBGColor = atoi_base(v, 16);

	v = sconf_get(sconf, "frame_wide");
	if(v[0] != 0) 
		frameW = atoi(v);

	v = sconf_get(sconf, "title_h");
	if(v[0] != 0) 
		titleH = atoi(v);

	int font_size = 14;
	v = sconf_get(sconf, "font_size");
	if(v[0] != 0) 
		font_size = atoi(v);

	v = sconf_get(sconf, "font");
	if(v[0] != 0) 
 		font_load(v, font_size, &font);
	else
 		font_load("/data/fonts/system.ttf", font_size, &font);

	sconf_free(sconf);
}

void SolarisWM::getColor(uint32_t *fg, uint32_t* bg, bool top) {
	if(top) {
		*fg = fgTopColor;
		*bg = bgTopColor;
	}
	else {
		*fg = fgColor;
		*bg = bgColor;
	}
}

SolarisWM::~SolarisWM(void) {
	font_close(&font);
	if(bgImg == NULL)
		return;
	graph_free(bgImg);
}

SolarisWM::SolarisWM(void) {
	desktopBGColor = 0xff555588;
	desktopFGColor = 0xff8888aa;
	bgColor = 0xff666666;
	fgColor = 0xff888888;
	bgTopColor = 0xffaaaaaa;
	fgTopColor = 0xff222222;
	frameW = 4;
	bgImg = NULL;
	titleH = 24;
}