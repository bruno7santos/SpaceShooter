/*
 * player.h — A nave do jogador
 */
#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>
#include "raylib.h"
#include "projectile.h"

/* Resultado de uma tentativa de causar dano ao jogador */
typedef enum {
    HIT_IGNORED,          /* estava invulnerável: nada acontece */
    HIT_SHIELD_BLOCKED,   /* o escudo absorveu o golpe */
    HIT_LIFE_LOST         /* perdeu uma vida */
} HitResult;

typedef struct Player {
    Vector2 position;
    Vector2 velocity;
    float radius;          /* raio da colisão */
    int lives;
    int weaponLevel;       /* 1 a PLAYER_MAX_WEAPON */
    float fireCooldown;    /* tempo até poder atirar de novo */
    float invulnTimer;     /* > 0: invulnerável (a nave pisca) */
    float shieldTimer;     /* > 0: escudo ativo */
    float animTime;        /* usado nas animações (chama do motor) */
    bool alive;
} Player;

void      Player_Init(Player *player);
/* Lê o teclado, move a nave e atira (criando projéteis em "projectiles"). */
void      Player_Update(Player *player, float dt, Projectile *projectiles, int maxProjectiles);
void      Player_Draw(const Player *player);

/* Desenha apenas a silhueta da nave (também usada nos ícones de vida e no menu). */
void      Player_DrawShip(Vector2 position, float scale, float alpha, bool flame, float time);

HitResult Player_TakeDamage(Player *player);
bool      Player_CanBeHit(const Player *player);
bool      Player_AddLife(Player *player);        /* false se já estiver no máximo */
bool      Player_UpgradeWeapon(Player *player);  /* false se já estiver no nível máximo */
void      Player_GiveShield(Player *player);

#endif /* PLAYER_H */
