/*
 * game.h — Estrutura principal do jogo, estados e regras
 */
#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include "raylib.h"
#include "background.h"
#include "config.h"
#include "enemy.h"
#include "menu.h"
#include "particles.h"
#include "player.h"
#include "powerup.h"
#include "projectile.h"
#include "wave.h"

/* Estados do jogo (máquina de estados) */
typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER,
    STATE_VICTORY
} GameState;

typedef struct Game {
    GameState state;
    bool shouldQuit;           /* true encerra o laço principal */
    bool showFps;

    /* entidades */
    Player player;
    Enemy enemies[MAX_ENEMIES];
    Projectile projectiles[MAX_PROJECTILES];
    PowerUp powerUps[MAX_POWERUPS];
    ParticleSystem particles;
    Background background;
    WaveState wave;

    /* placar */
    int score;
    int highScore;
    bool newRecord;            /* a partida atual bateu o recorde? */
    int victoryBonus;

    /* tempo e efeitos visuais */
    float time;                /* tempo total (animações de menu) */
    float runTime;             /* tempo da partida atual */
    float shake;               /* intensidade do tremor de tela, em pixels */
    Vector2 shakeOffset;
    float damageFlash;         /* 1 -> 0: flash vermelho ao levar dano */

    /* sequências de fim de partida */
    float gameOverTimer;       /* > 0: nave destruída, esperando mostrar o Game Over */
    float victoryTimer;        /* > 0: chefe derrotado, esperando mostrar a Vitória */
    float blastTimer;
    Vector2 bossPos;

    /* menus */
    Menu mainMenu;
    Menu pauseMenu;
    Menu endMenu;              /* usado nas telas de Game Over e Vitória */
} Game;

void Game_Init(Game *game);
void Game_Update(Game *game, float dt);
void Game_Draw(const Game *game);
void Game_Unload(Game *game);

/* ---- "Eventos" disparados pelo módulo de colisão (regras do jogo) ---- */
void Game_OnEnemyHit(Game *game, Enemy *enemy, int damage, Vector2 hitPosition);
void Game_OnEnemyRammed(Game *game, Enemy *enemy);
void Game_OnPlayerHit(Game *game);
void Game_OnPowerUpCollected(Game *game, PowerUp *powerUp);

#endif /* GAME_H */
