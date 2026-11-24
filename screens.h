/*
 * screens.h — Telas completas: menu principal, pausa, Game Over e Vitória
 */
#ifndef SCREENS_H
#define SCREENS_H

#include "game.h"

void Screen_DrawMenu(const Game *game);
void Screen_DrawPause(const Game *game);
void Screen_DrawGameOver(const Game *game);
void Screen_DrawVictory(const Game *game);

#endif /* SCREENS_H */
