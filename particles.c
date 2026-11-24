/*
 * particles.c — Sistema de partículas
 * As partículas ficam em um buffer circular: se o limite for atingido, a mais
 * antiga é reaproveitada. Assim o array nunca estoura e o custo de criar uma
 * partícula é constante.
 */
#include "particles.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include "gfx.h"
#include "utils.h"

static Particle *NextParticle(ParticleSystem *ps)
{
    Particle *p = &ps->particles[ps->cursor];
    ps->cursor = (ps->cursor + 1) % MAX_PARTICLES;
    memset(p, 0, sizeof *p);
    p->active = true;
    return p;
}

void Particles_Init(ParticleSystem *ps)
{
    memset(ps, 0, sizeof *ps);
}

void Particles_Clear(ParticleSystem *ps)
{
    memset(ps, 0, sizeof *ps);
}

void Particles_Explosion(ParticleSystem *ps, Vector2 position, Color color, int count, float speed)
{
    /* faíscas: muitas, rápidas, em todas as direções */
    for (int i = 0; i < count; i++) {
        Particle *p = NextParticle(ps);
        float angle = RandFloat(0.0f, 2.0f * PI);
        float v = RandFloat(0.2f, 1.0f) * speed;

        p->streak = (i % 2 == 0);
        p->position = position;
        p->velocity = (Vector2){ cosf(angle) * v, sinf(angle) * v };
        p->maxLife = p->life = RandFloat(0.35f, 0.90f);
        p->size = RandFloat(2.0f, 4.5f);
        p->drag = 2.2f;
        p->color = (i % 3 == 0) ? COL_WHITE_SOFT : ColorMix(color, COL_ORANGE, RandFloat(0.0f, 0.5f));
    }

    /* bolas de fogo: poucas, grandes e lentas */
    for (int i = 0; i < count / 4; i++) {
        Particle *p = NextParticle(ps);
        float angle = RandFloat(0.0f, 2.0f * PI);
        float v = RandFloat(0.0f, 0.35f) * speed;

        p->streak = false;
        p->position = (Vector2){ position.x + RandFloat(-6.0f, 6.0f), position.y + RandFloat(-6.0f, 6.0f) };
        p->velocity = (Vector2){ cosf(angle) * v, sinf(angle) * v };
        p->maxLife = p->life = RandFloat(0.30f, 0.60f);
        p->size = RandFloat(9.0f, 20.0f);
        p->drag = 2.5f;
        p->color = ColorMix(COL_ORANGE, color, RandFloat(0.0f, 0.5f));
    }
}

void Particles_Sparks(ParticleSystem *ps, Vector2 position, Color color, int count)
{
    for (int i = 0; i < count; i++) {
        Particle *p = NextParticle(ps);
        float angle = RandFloat(0.0f, 2.0f * PI);
        float v = RandFloat(120.0f, 320.0f);

        p->streak = true;
        p->position = position;
        p->velocity = (Vector2){ cosf(angle) * v, sinf(angle) * v };
        p->maxLife = p->life = RandFloat(0.15f, 0.35f);
        p->size = RandFloat(1.5f, 3.0f);
        p->drag = 3.0f;
        p->color = (i % 2 == 0) ? color : COL_WHITE_SOFT;
    }
}

/* Chama do motor da nave: emite ~100 partículas por segundo, independente do FPS. */
void Particles_Thruster(ParticleSystem *ps, Vector2 shipPosition, float dt)
{
    ps->emitAccum += dt * 100.0f;

    while (ps->emitAccum >= 1.0f) {
        Particle *p = NextParticle(ps);
        ps->emitAccum -= 1.0f;

        p->streak = false;
        p->position = (Vector2){ shipPosition.x + RandFloat(-4.0f, 4.0f), shipPosition.y + 26.0f };
        p->velocity = (Vector2){ RandFloat(-18.0f, 18.0f), RandFloat(120.0f, 220.0f) };
        p->maxLife = p->life = RandFloat(0.18f, 0.34f);
        p->size = RandFloat(2.5f, 4.5f);
        p->drag = 1.0f;
        p->color = ColorMix(COL_ORANGE, COL_GOLD, RandFloat(0.0f, 1.0f));
    }
}

void Particles_AddPopup(ParticleSystem *ps, Vector2 position, const char *text, Color color)
{
    ScorePopup *p = &ps->popups[ps->popupCursor];
    ps->popupCursor = (ps->popupCursor + 1) % MAX_POPUPS;

    p->active = true;
    p->position = position;
    p->maxLife = p->life = 0.95f;
    p->color = color;
    snprintf(p->text, sizeof p->text, "%s", text);     /* copia truncando se for longo demais */
}

void Particles_Update(ParticleSystem *ps, float dt)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        Particle *p = &ps->particles[i];
        if (!p->active) continue;

        p->life -= dt;
        if (p->life <= 0.0f) {
            p->active = false;
            continue;
        }

        float damping = 1.0f - p->drag * dt;
        if (damping < 0.0f) damping = 0.0f;
        p->velocity = Vector2Scale(p->velocity, damping);
        p->position = Vector2Add(p->position, Vector2Scale(p->velocity, dt));
    }

    for (int i = 0; i < MAX_POPUPS; i++) {
        ScorePopup *p = &ps->popups[i];
        if (!p->active) continue;

        p->life -= dt;
        if (p->life <= 0.0f) {
            p->active = false;
            continue;
        }
        p->position.y -= 55.0f * dt;
    }
}

void Particles_Draw(const ParticleSystem *ps)
{
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < MAX_PARTICLES; i++) {
        const Particle *p = &ps->particles[i];
        if (!p->active) continue;

        float t = p->life / p->maxLife;           /* 1 -> 0 ao longo da vida */
        Color c = ColorWithAlpha(p->color, t);

        if (p->streak) {
            Vector2 tail = Vector2Subtract(p->position, Vector2Scale(p->velocity, 0.045f));
            DrawLineEx(tail, p->position, p->size * (0.4f + 0.6f * t), c);
        } else {
            DrawCircleV(p->position, p->size * (0.3f + 0.7f * t), c);
        }
    }
    EndBlendMode();

    for (int i = 0; i < MAX_POPUPS; i++) {
        const ScorePopup *p = &ps->popups[i];
        if (!p->active) continue;

        float t = p->life / p->maxLife;
        Gfx_DrawTextCentered(p->text, p->position.x, p->position.y, 20.0f, ColorWithAlpha(p->color, fminf(1.0f, t * 2.0f)));
    }
}
