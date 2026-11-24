/*
 * projectile.h — Projéteis (tiros do jogador e dos inimigos)
 * Todos os tiros ficam em um único array; o campo "owner" diz de quem é cada um.
 */
#ifndef PROJECTILE_H
#define PROJECTILE_H

#include <stdbool.h>
#include "raylib.h"

typedef enum {
    OWNER_PLAYER,
    OWNER_ENEMY
} ProjectileOwner;

typedef struct Projectile {
    bool active;
    ProjectileOwner owner;
    Vector2 position;
    Vector2 velocity;     /* pixels por segundo */
    float radius;         /* raio da colisão */
    int damage;
    Color color;
} Projectile;

/* Cria um tiro no primeiro espaço livre. Devolve false se o array estiver cheio (o tiro é descartado). */
bool Projectile_Spawn(Projectile *list, int max, Vector2 position, Vector2 velocity,
                      float radius, int damage, ProjectileOwner owner, Color color);
void Projectile_UpdateAll(Projectile *list, int max, float dt);
void Projectile_DrawAll(const Projectile *list, int max);
void Projectile_ClearAll(Projectile *list, int max);
void Projectile_ClearOwner(Projectile *list, int max, ProjectileOwner owner);

#endif /* PROJECTILE_H */
