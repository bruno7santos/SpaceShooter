/*
 * wave.c — Controle das ondas e do surgimento de inimigos
 */
#include "wave.h"

#include <stddef.h>

#include "config.h"
#include "utils.h"

#define NORMAL_TYPES 5     /* SCOUT, ZIGZAG, KAMIKAZE, TANK, SHOOTER (mesma ordem de EnemyType) */

/* Pesos de sorteio por onda: quanto maior o peso, mais frequente o inimigo.
 * Colunas: SCOUT, ZIGZAG, KAMIKAZE, TANK, SHOOTER. Novos tipos vão surgindo com as ondas. */
static const int SPAWN_WEIGHTS[5][NORMAL_TYPES] = {
    /* onda 1 */ { 80, 20,  0,  0,  0 },
    /* onda 2 */ { 50, 35, 15,  0,  0 },
    /* onda 3 */ { 30, 25, 15, 30,  0 },   /* chegam os couraçados */
    /* onda 4 */ { 22, 20, 15, 18, 25 },   /* chegam os atiradores */
    /* onda 5 */ { 12, 18, 20, 20, 30 }
};

/* Durante a luta contra o chefe só aparecem reforços leves. */
static const int BOSS_WAVE_WEIGHTS[NORMAL_TYPES] = { 50, 30, 20, 0, 0 };

/* ------------------------------------------------------------------ */
/*  Pontuação necessária para cada onda                                */
/* ------------------------------------------------------------------ */

static int WaveQuota(int wave)                  /* pontos que esta onda exige */
{
    return WAVE_BASE_SCORE + WAVE_SCORE_STEP * (wave - 1);
}

static int WaveStartScore(int wave)             /* pontuação total em que a onda começa */
{
    int total = 0;
    for (int w = 1; w < wave; w++) total += WaveQuota(w);
    return total;
}

static int WaveEndScore(int wave)
{
    return WaveStartScore(wave) + WaveQuota(wave);
}

float Wave_Progress(const WaveState *w, int score)
{
    float p = (float)(score - WaveStartScore(w->wave)) / (float)WaveQuota(w->wave);
    return Clamp(p, 0.0f, 1.0f);
}

/* Dificuldade contínua: onda-1 + progresso dentro da onda. Sobe junto com a pontuação. */
float Wave_Difficulty(const WaveState *w, int score)
{
    return (float)(w->wave - 1) + Wave_Progress(w, score);
}

/* ------------------------------------------------------------------ */
/*  Geração de inimigos                                                */
/* ------------------------------------------------------------------ */

static EnemyType PickEnemyType(int wave)
{
    const int *weights;

    if (wave >= FINAL_WAVE) {
        weights = BOSS_WAVE_WEIGHTS;
    } else {
        int row = wave - 1;
        if (row > 4) row = 4;
        weights = SPAWN_WEIGHTS[row];
    }

    int total = 0;
    for (int i = 0; i < NORMAL_TYPES; i++) total += weights[i];

    int roll = GetRandomValue(0, total - 1);
    for (int i = 0; i < NORMAL_TYPES; i++) {
        if (roll < weights[i]) return (EnemyType)i;
        roll -= weights[i];
    }
    return ENEMY_SCOUT;
}

/* Formação de batedores lado a lado ("linha") ou em "V", como nos clássicos arcade. */
static void SpawnFormation(Enemy *enemies, int max, float difficulty, bool vShape)
{
    int count = GetRandomValue(3, 5);
    float spacing = 95.0f;
    float width = spacing * (float)(count - 1);
    float centerX = RandFloat(70.0f + width * 0.5f, (float)SCREEN_WIDTH - 70.0f - width * 0.5f);

    for (int i = 0; i < count; i++) {
        float x = centerX - width * 0.5f + spacing * (float)i;
        float offset = vShape ? fabsf((float)i - (float)(count - 1) * 0.5f) * 50.0f : 0.0f;
        Enemy_Spawn(enemies, max, ENEMY_SCOUT, x, -40.0f - offset, difficulty);
    }
}

static void SpawnOne(Enemy *enemies, int max, int wave, float difficulty)
{
    EnemyType type = PickEnemyType(wave);
    float x = RandFloat(60.0f, (float)SCREEN_WIDTH - 60.0f);
    Enemy_Spawn(enemies, max, type, x, -40.0f, difficulty);
}

static float SpawnInterval(int wave, float difficulty)
{
    if (wave >= FINAL_WAVE) return 2.4f;                       /* poucos reforços durante o chefe */
    return Clamp(1.35f - 0.14f * difficulty, 0.50f, 1.35f);    /* quanto mais difícil, mais rápido */
}

/* ------------------------------------------------------------------ */
/*  API                                                                */
/* ------------------------------------------------------------------ */

void Wave_Init(WaveState *w)
{
    w->wave = 1;
    w->spawnTimer = 0.0f;
    w->bannerTimer = WAVE_BANNER_TIME;
    w->restTimer = WAVE_REST_TIME;
    w->bossSpawned = false;
}

WaveEvent Wave_Update(WaveState *w, float dt, int score, Enemy *enemies, int maxEnemies, bool spawnAllowed)
{
    WaveEvent event = WAVE_EVENT_NONE;

    if (w->bannerTimer > 0.0f) w->bannerTimer -= dt;
    if (w->restTimer > 0.0f) w->restTimer -= dt;

    /* avança de onda ao atingir a pontuação exigida */
    if (w->wave < FINAL_WAVE && score >= WaveEndScore(w->wave)) {
        w->wave++;
        w->bannerTimer = WAVE_BANNER_TIME;
        w->restTimer = (w->wave >= FINAL_WAVE) ? WAVE_REST_TIME + 1.5f : WAVE_REST_TIME;
        w->spawnTimer = 0.0f;
        event = (w->wave >= FINAL_WAVE) ? WAVE_EVENT_FINAL_WAVE : WAVE_EVENT_NEW_WAVE;
    }

    if (!spawnAllowed || w->restTimer > 0.0f) return event;

    float difficulty = Wave_Difficulty(w, score);

    /* onda final: o chefe entra uma única vez */
    if (w->wave >= FINAL_WAVE && !w->bossSpawned) {
        if (Enemy_Spawn(enemies, maxEnemies, ENEMY_BOSS, SCREEN_WIDTH * 0.5f, -90.0f, difficulty) != NULL) {
            w->bossSpawned = true;
            event = WAVE_EVENT_BOSS_SPAWNED;
        }
    }

    /* inimigos comuns, em intervalos que diminuem com a dificuldade */
    w->spawnTimer -= dt;
    if (w->spawnTimer <= 0.0f) {
        int alive = Enemy_CountActive(enemies, maxEnemies);
        int limit = (w->wave >= FINAL_WAVE) ? 8 : MAX_ALIVE_ENEMIES;

        if (alive < limit) {
            bool formation = (w->wave >= 2 && w->wave < FINAL_WAVE && GetRandomValue(0, 99) < 16);
            if (formation) {
                SpawnFormation(enemies, maxEnemies, difficulty, GetRandomValue(0, 1) == 0);
                w->spawnTimer = SpawnInterval(w->wave, difficulty) * 2.2f;   /* formações "gastam" mais tempo */
            } else {
                SpawnOne(enemies, maxEnemies, w->wave, difficulty);
                w->spawnTimer = SpawnInterval(w->wave, difficulty);
            }
        } else {
            w->spawnTimer = 0.3f;     /* tela cheia: tenta de novo daqui a pouco */
        }
    }
    return event;
}
