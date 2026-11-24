/*
 * background.h — Fundo espacial: degradê, nebulosas e estrelas com parallax
 */
#ifndef BACKGROUND_H
#define BACKGROUND_H

#include "raylib.h"
#include "config.h"

typedef struct Star {
    float x, y;
    int layer;            /* 0 = distante (lenta), 1 = média, 2 = próxima (rápida) */
    float twinkle;        /* fase da cintilação */
} Star;

typedef struct Nebula {
    float x, y;
    float radius;
    float speed;
    Color color;
} Nebula;

typedef struct Background {
    Star stars[STAR_COUNT];
    Nebula nebulas[NEBULA_COUNT];
    float time;
} Background;

void Background_Init(Background *bg);
/* speedScale: 1.0 = velocidade normal de rolagem */
void Background_Update(Background *bg, float dt, float speedScale);
void Background_Draw(const Background *bg);

#endif /* BACKGROUND_H */
