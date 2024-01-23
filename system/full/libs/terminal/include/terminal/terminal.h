#ifndef terminal_H
#define terminal_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t UNICODE16;

typedef struct {
	UNICODE16 c;
	uint32_t color;
} tchar_t;

typedef struct {
	tchar_t* content;
	uint32_t cols;
	uint32_t rows;
	uint32_t curs_x;
	uint32_t curs_y;
} terminal_t;

void      terminal_init(terminal_t* terminal);
void      terminal_clear(terminal_t* terminal);
void      terminal_close(terminal_t* terminal);
void      terminal_reset(terminal_t* terminal, uint32_t cols, uint32_t rows);

void      terminal_move_to(terminal_t* terminal, uint32_t x, uint32_t y);
void      terminal_move_at(terminal_t* terminal, uint32_t at);
void      terminal_move(terminal_t* terminal, int32_t steps);
void      terminal_move_next_line(terminal_t* terminal);

int32_t   terminal_pos_by_at(terminal_t* terminal, uint32_t at, uint32_t *x, uint32_t *y);
int32_t   terminal_pos(terminal_t* terminal, uint32_t *x, uint32_t *y);
uint32_t  terminal_at_by_pos(terminal_t* terminal, uint32_t x, uint32_t y);
uint32_t  terminal_at(terminal_t* terminal);

tchar_t*  terminal_get(terminal_t* terminal);
tchar_t*  terminal_get_by_at(terminal_t* terminal, uint32_t at);
tchar_t*  terminal_get_by_pos(terminal_t* terminal, uint32_t x, uint32_t y);

uint32_t  terminal_size(terminal_t* terminal);

void      terminal_set(terminal_t* terminal, UNICODE16 ci, uint32_t color);

#ifdef __cplusplus
}
#endif

#endif
