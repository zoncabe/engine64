#ifndef ENGINE64_MENU_H
#define ENGINE64_MENU_H

#include <stdint.h>

#define MENU_STACK_MAX 4


/* Where the cursor stands on one level of a menu, and how far it can go.
   Opening a submenu pushes a level and remembers the one below. */
typedef struct {

	int8_t index;
	int8_t item_count;

} MenuStackFrame;

typedef struct {

	MenuStackFrame frame[MENU_STACK_MAX];
	uint8_t        top;
	int8_t         index;

} MenuStack;


void menuStack_init(void);
void menuStack_open(int8_t item_count);
void menuStack_back(void);

int8_t menuStack_getIndex(void);
void   menuStack_setIndex(int8_t index);
void   menuStack_moveIndex(int8_t delta, int8_t max);

int8_t menuStack_getItemCount(void);

#endif
