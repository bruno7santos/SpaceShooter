/*
 * config.h — Constantes do jogo
 * ------------------------------------------------------------------
 * Todos os "números mágicos" importantes ficam aqui. Para ajustar a
 * dificuldade ou o "game feel", basta alterar estes valores.
 */
#ifndef CONFIG_H
#define CONFIG_H

/* ---------- Janela ---------- */
#define SCREEN_WIDTH      1280
#define SCREEN_HEIGHT     720
#define TARGET_FPS        60
#define GAME_TITLE        "Space Shooter"
#define MAX_FRAME_TIME    0.05f   /* limite do delta time (evita "saltos" se a janela travar) */

/* ---------- Tamanho dos arrays (pools de entidades) ---------- */
#define MAX_ENEMIES       48
#define MAX_PROJECTILES   320
#define MAX_POWERUPS      6
#define MAX_PARTICLES     900
#define MAX_POPUPS        16
#define STAR_COUNT        140
#define NEBULA_COUNT      4

/* ---------- Jogador ---------- */
#define PLAYER_START_LIVES   3
#define PLAYER_MAX_LIVES     5
#define PLAYER_MAX_WEAPON    3
#define PLAYER_SPEED         470.0f   /* pixels por segundo */
#define PLAYER_ACCEL         14.0f    /* suavização do movimento: maior = resposta mais "seca" */
#define PLAYER_RADIUS        12.0f    /* raio da colisão (menor que o desenho, como nos shmups clássicos) */
#define PLAYER_MARGIN        34.0f    /* distância mínima até as bordas da tela */
#define PLAYER_INVULN_TIME   2.0f     /* segundos de invulnerabilidade após perder uma vida */
#define PLAYER_SHIELD_TIME   10.0f    /* duração do escudo (power-up) */

/* ---------- Tiros ---------- */
#define BULLET_SPEED          860.0f
#define BULLET_RADIUS         5.0f
#define ENEMY_BULLET_SPEED    290.0f
#define ENEMY_BULLET_RADIUS   6.0f

/* ---------- Ondas e dificuldade ---------- */
#define FINAL_WAVE             6       /* última onda: luta contra o chefe */
#define WAVE_BASE_SCORE        3000    /* pontos necessários para completar a onda 1 */
#define WAVE_SCORE_STEP        800     /* cada onda seguinte exige este tanto a mais */
#define WAVE_BANNER_TIME       3.0f    /* tempo do aviso "ONDA N" na tela */
#define WAVE_REST_TIME         2.2f    /* pausa sem inimigos ao iniciar cada onda */
#define MAX_ALIVE_ENEMIES      16      /* limite de inimigos simultâneos */
#define DIFFICULTY_SPEED_STEP  0.07f   /* +7% de velocidade a cada "nível" de dificuldade */

/* ---------- Pontuação e fim de jogo ---------- */
#define VICTORY_LIFE_BONUS  1000       /* bônus por vida restante ao vencer */
#define GAME_OVER_DELAY     1.8f       /* tempo entre a explosão da nave e a tela de Game Over */
#define VICTORY_DELAY       3.2f       /* tempo entre a morte do chefe e a tela de Vitória */
#define HIGHSCORE_FILE      "highscore.dat"

#endif /* CONFIG_H */
