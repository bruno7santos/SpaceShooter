/*
 * enemy.h — Inimigos (cinco tipos comuns e o chefe final)
 */
#ifndef ENEMY_H
#define ENEMY_H

#include <stdbool.h>
#include "raylib.h"
#include "projectile.h"

/* ATENÇÃO: a ordem importa. wave.c usa estes valores como índices da tabela de sorteio. */
typedef enum {
    ENEMY_SCOUT = 0,   /* batedor: desce em linha reta, 1 de vida */
    ENEMY_ZIGZAG,      /* ondulante: desce em zigue-zague */
    ENEMY_KAMIKAZE,    /* rápido: persegue o jogador pelos lados */
    ENEMY_TANK,        /* couraçado: lento e muito resistente */
    ENEMY_SHOOTER,     /* atirador: para no alto e atira mirando no jogador */
    ENEMY_BOSS,        /* chefe final: várias fases de ataque */
    ENEMY_TYPE_COUNT
} EnemyType;

typedef struct Enemy {
    bool active;
    EnemyType type;
    Vector2 position;
    Vector2 velocity;      /* pixels por segundo */
    float radius;          /* raio da colisão */
    int hp;
    int maxHp;
    int points;            /* pontos ao ser destruído */
    float speedScale;      /* multiplicador de velocidade herdado da dificuldade */
    float age;             /* tempo de vida (usado em movimentos senoidais e animações) */
    float baseX;           /* eixo central do movimento horizontal */
    float phase;           /* fase do movimento senoidal */
    float shootTimer;      /* tempo até o próximo disparo */
    float burstTimer;      /* tempo até a próxima rajada circular (chefe) */
    float stateTimer;
    float hoverY;          /* altura em que o atirador para */
    int state;             /* sub-estado (ex.: atirador: 0 descendo, 1 patrulhando, 2 saindo) */
    float hitFlash;        /* > 0: pisca em branco após levar um tiro */
} Enemy;

/* Informações que os inimigos precisam do "mundo" para agir */
typedef struct EnemyContext {
    Vector2 targetPos;         /* posição do jogador (para mirar e perseguir) */
    Projectile *projectiles;   /* array onde os inimigos criam seus tiros */
    int maxProjectiles;
} EnemyContext;

/*
 * Cria um inimigo no primeiro espaço livre (devolve NULL se o array estiver cheio).
 * difficulty >= 0 aumenta a velocidade e a resistência de alguns tipos.
 */
Enemy *Enemy_Spawn(Enemy *list, int max, EnemyType type, float x, float y, float difficulty);
void   Enemy_UpdateAll(Enemy *list, int max, float dt, const EnemyContext *ctx);
void   Enemy_DrawAll(const Enemy *list, int max);
void   Enemy_ClearAll(Enemy *list, int max);

/* Aplica dano. Devolve true se o inimigo foi destruído. */
bool   Enemy_Hit(Enemy *enemy, int damage);
bool   Enemy_IsVulnerable(const Enemy *enemy);   /* o chefe é invulnerável enquanto entra na tela */
int    Enemy_CountActive(const Enemy *list, int max);
const Enemy *Enemy_FindBoss(const Enemy *list, int max);
int    Enemy_BossPhase(const Enemy *enemy);      /* 1, 2 ou 3 conforme a vida restante do chefe */
Color  Enemy_Color(EnemyType type);

/* Desenha um inimigo "de enfeite" (ex.: no menu principal). */
void   Enemy_DrawPreview(EnemyType type, Vector2 position, float scale, float time);

#endif /* ENEMY_H */
