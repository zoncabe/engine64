#ifndef UI_H
#define UI_H

typedef struct Game Game;


const RenderContext* mainMenu_getRenderContext(void);
const RenderContext* pause_getRenderContext(void);

void mainMenu_animate();
void pause_animate(const Game *game);

#endif
