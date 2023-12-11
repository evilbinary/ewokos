#ifndef GRAPH_EX_H
#define GRAPH_EX_H

#include <graph/graph.h>

#ifdef __cplusplus
extern "C" {
#endif

void graph_get_3d_color(uint32_t base, uint32_t *dark, uint32_t *bright);
void graph_box_3d(graph_t* g, int x, int y, int w, int h, uint32_t bright_color, uint32_t dark_color);
void graph_frame(graph_t* g, int x, int y, int w, int h, int width, uint32_t base_color, bool rev);
void graph_fill_3d(graph_t* g, int x, int y, int w, int h, uint32_t color, bool rev);
void graph_draw_dot_pattern(graph_t* g,int x, int y, int w, int h, uint32_t c1, uint32_t c2);
void graph_gradation(graph_t* g,int x, int y, int w, int h, uint32_t c1, uint32_t c2, bool vertical);

#ifdef __cplusplus
}
#endif

#endif
