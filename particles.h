/*
 * particles.h — Partículas (explosões, faíscas, chama do motor) e textos flutuantes
 */
#ifndef PARTICLES_H
#define PARTICLES_H

#include <stdbool.h>
#include "raylib.h"
#include "config.h"

typedef struct Particle {
    bool active;
    bool streak;        /* true = faísca (risco); false = bola de fogo/fumaça */
    Vector2 position;
    Vector2 velocity;
    float life;         /* tempo restante */
    float maxLife;
    float size;
    float drag;         /* atrito: reduz a velocidade ao longo do tempo */
    Color color;
} Particle;

/* Texto que sobe e some (ex.: "+100") */
typedef struct ScorePopup {
    bool active;
    Vector2 position;
    float life;
    float maxLife;
    Color color;
    char text[24];
} ScorePopup;

typedef struct ParticleSystem {
    Particle particles[MAX_PARTICLES];
    int cursor;                        /* próximo índice a usar (buffer circular) */
    ScorePopup popups[MAX_POPUPS];
    int popupCursor;
    float emitAccum;                   /* acumulador para emitir a chama do motor */
} ParticleSystem;

void Particles_Init(ParticleSystem *ps);
void Particles_Clear(ParticleSystem *ps);

void Particles_Explosion(ParticleSystem *ps, Vector2 position, Color color, int count, float speed);
void Particles_Sparks(ParticleSystem *ps, Vector2 position, Color color, int count);
void Particles_Thruster(ParticleSystem *ps, Vector2 shipPosition, float dt);
void Particles_AddPopup(ParticleSystem *ps, Vector2 position, const char *text, Color color);

void Particles_Update(ParticleSystem *ps, float dt);
void Particles_Draw(const ParticleSystem *ps);

#endif /* PARTICLES_H */
