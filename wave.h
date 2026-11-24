/*
 * wave.h — Ondas, dificuldade progressiva e geração (spawn) de inimigos
 * ------------------------------------------------------------------
 * A onda atual depende da PONTUAÇÃO: cada onda exige uma quantidade de pontos
 * (WAVE_BASE_SCORE + WAVE_SCORE_STEP por onda). Ao atingi-la, a próxima onda
 * começa com um aviso na tela. A última onda (FINAL_WAVE) traz o chefe.
 */
#ifndef WAVE_H
#define WAVE_H

#include <stdbool.h>
#include "enemy.h"

typedef struct WaveState {
    int wave;             /* onda atual (1 .. FINAL_WAVE) */
    float spawnTimer;     /* tempo até o próximo inimigo */
    float bannerTimer;    /* > 0: o aviso "ONDA N" está na tela */
    float restTimer;      /* > 0: pausa sem inimigos no começo da onda */
    bool bossSpawned;
} WaveState;

typedef enum {
    WAVE_EVENT_NONE,
    WAVE_EVENT_NEW_WAVE,       /* uma nova onda comum começou */
    WAVE_EVENT_FINAL_WAVE,     /* começou a onda final */
    WAVE_EVENT_BOSS_SPAWNED    /* o chefe acabou de aparecer */
} WaveEvent;

void  Wave_Init(WaveState *w);

/* Atualiza temporizadores, avança de onda conforme o score e cria inimigos (se spawnAllowed). */
WaveEvent Wave_Update(WaveState *w, float dt, int score, Enemy *enemies, int maxEnemies, bool spawnAllowed);

float Wave_Progress(const WaveState *w, int score);      /* 0..1 dentro da onda atual */
float Wave_Difficulty(const WaveState *w, int score);    /* 0, 1, 2... cresce de forma contínua */

#endif /* WAVE_H */
