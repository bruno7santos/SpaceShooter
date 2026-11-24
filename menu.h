/*
 * menu.h — Componente de menu reutilizável (menu principal, pausa, fim de jogo)
 * Funciona com teclado (setas/W/S + ENTER/ESPAÇO) e com o mouse.
 */
#ifndef MENU_H
#define MENU_H

#include "raylib.h"

#define MENU_MAX_ITEMS 4

typedef struct Menu {
    const char *items[MENU_MAX_ITEMS];
    int count;
    int selected;
    float centerX;       /* posição do menu na tela */
    float startY;
    float spacing;
    float inputLock;     /* > 0: ignora "confirmar" (evita apertar sem querer ao entrar na tela) */
} Menu;

void Menu_Init(Menu *menu, const char *const *labels, int count, float centerX, float startY, float spacing);
void Menu_Reset(Menu *menu, float lockSeconds);

/* Devolve o índice da opção confirmada neste frame, ou -1 se nenhuma. */
int  Menu_Update(Menu *menu, float dt);
void Menu_Draw(const Menu *menu, float time);

#endif /* MENU_H */
