#ifndef MENU_H
#define MENU_H

#include "../ui/ui.h"

#define MENU_STACK_MAX 4


typedef struct {

    int8_t   index;
    RenderContext *stack[MENU_STACK_MAX];
    uint8_t  top;

} Menu;

void menu_init();
void menu_open(RenderContext *screen);
void menu_back();

int8_t menu_getIndex(void);
void menu_setIndex(int8_t index);
void menu_moveIndex(int8_t delta, int8_t max);

RenderContext *menu_current();

#endif