#ifndef INTRO_H
#define INTRO_H

#define INTRO_SCENE_LENGTH 10.0f


const RenderContext* intro_getRenderContext(void);

void intro_init();
void intro_close();
void intro_animate(float t);
bool intro_update(void);

#endif
