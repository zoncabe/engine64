#ifndef MENU_H
#define MENU_H

#include <stdint.h>
#include "screen/screen.h"

#define MENU_STACK_MAX 4


typedef struct {

	Screen  *screen;
	int8_t   index;
	int8_t   item_count;

} MenuStackFrame;

typedef struct {

	MenuStackFrame frame[MENU_STACK_MAX];
	uint8_t        top;
	int8_t         index;

} MenuStack;


Screen *menuStack_current(void);


void menuStack_init(void);
void menuStack_open(Screen *screen, int8_t item_count);
void menuStack_back(void);

int8_t        menuStack_getIndex(void);
const int8_t *menuStack_getIndexPtr(void);
void          menuStack_setIndex(int8_t index);
void          menuStack_moveIndex(int8_t delta, int8_t max);

int8_t menuStack_getItemCount(void);

#endif
