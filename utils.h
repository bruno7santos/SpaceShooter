/*
 * utils.h — Pequenas funções utilitárias usadas por vários módulos.
 * São "static inline": ficam no próprio header e não precisam de .c.
 */
#ifndef UTILS_H
#define UTILS_H

#include <math.h>
#include <stdbool.h>
#include "raylib.h"
#include "raymath.h"

/* Número real aleatório entre min e max, usando o gerador da raylib. */
static inline float RandFloat(float min, float max)
{
    return min + (max - min) * ((float)GetRandomValue(0, 10000) / 10000.0f);
}

/* Devolve -1.0f ou +1.0f ao acaso. */
static inline float RandSign(void)
{
    return (GetRandomValue(0, 1) == 0) ? -1.0f : 1.0f;
}

/* Cor com a transparência trocada (alpha de 0.0 a 1.0). */
static inline Color ColorWithAlpha(Color c, float alpha)
{
    c.a = (unsigned char)(255.0f * Clamp(alpha, 0.0f, 1.0f));
    return c;
}

/* Mistura duas cores: t = 0 devolve a, t = 1 devolve b. */
static inline Color ColorMix(Color a, Color b, float t)
{
    Color r;
    t = Clamp(t, 0.0f, 1.0f);
    r.r = (unsigned char)((float)a.r + ((float)b.r - (float)a.r) * t);
    r.g = (unsigned char)((float)a.g + ((float)b.g - (float)a.g) * t);
    r.b = (unsigned char)((float)a.b + ((float)b.b - (float)a.b) * t);
    r.a = (unsigned char)((float)a.a + ((float)b.a - (float)a.a) * t);
    return r;
}

/* Verdadeiro durante metade de cada período: útil para efeitos de "piscar". */
static inline bool BlinkOn(float time, float period)
{
    return fmodf(time, period) < period * 0.5f;
}

#endif /* UTILS_H */
