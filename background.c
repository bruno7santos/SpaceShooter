/*
 * background.c — Fundo espacial animado
 */
#include "background.h"

#include <math.h>
#include "gfx.h"
#include "utils.h"

/* Valores por camada de estrelas (índice = layer) */
static const float STAR_SPEED[3] = { 22.0f, 60.0f, 150.0f };
static const float STAR_SIZE[3]  = { 1.5f, 2.0f, 3.0f };
static const float STAR_ALPHA[3] = { 0.45f, 0.70f, 1.00f };

void Background_Init(Background *bg)
{
    static const Color nebulaColors[NEBULA_COUNT] = {
        { 120,  60, 255, 255 },
        {  30, 140, 255, 255 },
        { 255,  60, 170, 255 },
        {  40, 200, 190, 255 }
    };

    bg->time = 0.0f;

    for (int i = 0; i < STAR_COUNT; i++) {
        bg->stars[i].x = RandFloat(0.0f, (float)SCREEN_WIDTH);
        bg->stars[i].y = RandFloat(0.0f, (float)SCREEN_HEIGHT);
        bg->stars[i].layer = i % 3;
        bg->stars[i].twinkle = RandFloat(0.0f, 2.0f * PI);
    }

    for (int i = 0; i < NEBULA_COUNT; i++) {
        bg->nebulas[i].x = RandFloat(0.0f, (float)SCREEN_WIDTH);
        bg->nebulas[i].y = RandFloat(-200.0f, (float)SCREEN_HEIGHT);
        bg->nebulas[i].radius = RandFloat(220.0f, 360.0f);
        bg->nebulas[i].speed = RandFloat(8.0f, 16.0f);
        bg->nebulas[i].color = nebulaColors[i];
    }
}

void Background_Update(Background *bg, float dt, float speedScale)
{
    bg->time += dt;

    for (int i = 0; i < STAR_COUNT; i++) {
        Star *s = &bg->stars[i];
        s->y += STAR_SPEED[s->layer] * speedScale * dt;
        if (s->y > (float)SCREEN_HEIGHT + 6.0f) {      /* saiu por baixo: reaparece no topo */
            s->y = -6.0f;
            s->x = RandFloat(0.0f, (float)SCREEN_WIDTH);
        }
    }

    for (int i = 0; i < NEBULA_COUNT; i++) {
        Nebula *n = &bg->nebulas[i];
        n->y += n->speed * speedScale * dt;
        if (n->y - n->radius > (float)SCREEN_HEIGHT) {
            n->y = -n->radius;
            n->x = RandFloat(0.0f, (float)SCREEN_WIDTH);
        }
    }
}

void Background_Draw(const Background *bg)
{
    /* degradê vertical feito com faixas horizontais */
    const Color top = { 4, 6, 20, 255 };
    const Color bottom = { 22, 10, 48, 255 };
    const int bands = 30;
    const int bandHeight = SCREEN_HEIGHT / bands;

    for (int i = 0; i < bands; i++) {
        DrawRectangle(0, i * bandHeight, SCREEN_WIDTH, bandHeight + 1,
                      ColorMix(top, bottom, (float)i / (float)(bands - 1)));
    }

    /* nebulosas: círculos concêntricos suaves somados à imagem (blend aditivo) */
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < NEBULA_COUNT; i++) {
        const Nebula *n = &bg->nebulas[i];
        Vector2 center = { n->x, n->y };
        for (int layer = 0; layer < 8; layer++) {          /* mais camadas = degradê mais suave */
            float f = 1.0f - (float)layer * 0.11f;
            DrawCircleV(center, n->radius * f, ColorWithAlpha(n->color, 0.014f));
        }
    }
    EndBlendMode();

    /* estrelas */
    for (int i = 0; i < STAR_COUNT; i++) {
        const Star *s = &bg->stars[i];
        float twinkle = 0.7f + 0.3f * sinf(bg->time * (1.5f + (float)s->layer) + s->twinkle);
        Color c = ColorWithAlpha(COL_WHITE_SOFT, STAR_ALPHA[s->layer] * twinkle);
        float size = STAR_SIZE[s->layer];

        if (s->layer == 2) {   /* estrelas próximas ganham um rastro de velocidade */
            DrawLineEx((Vector2){ s->x, s->y }, (Vector2){ s->x, s->y - 9.0f }, size * 0.7f, ColorWithAlpha(c, 0.35f));
        }
        DrawRectangleV((Vector2){ s->x - size * 0.5f, s->y - size * 0.5f }, (Vector2){ size, size }, c);
    }
}
