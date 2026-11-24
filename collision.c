/*
 * collision.c — Detecção de colisões (círculo x círculo)
 * Todas as entidades usam um círculo de colisão: é simples, rápido e funciona
 * bem em jogos de nave. O raio da colisão é menor que o desenho para o jogo
 * parecer "justo" (você só é atingido quando o tiro realmente acerta a nave).
 */
#include "collision.h"

#include "game.h"

#define POWERUP_PICKUP_BONUS 14.0f   /* "ímã": facilita pegar os itens */

static bool PlayerVulnerable(const Game *g)
{
    return Player_CanBeHit(&g->player) && g->victoryTimer <= 0.0f;
}

void Collision_Update(struct Game *game)
{
    Game *g = game;

    /* 1) tiros do jogador x inimigos */
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *b = &g->projectiles[i];
        if (!b->active || b->owner != OWNER_PLAYER) continue;

        for (int j = 0; j < MAX_ENEMIES; j++) {
            Enemy *e = &g->enemies[j];
            if (!e->active) continue;
            if (!CheckCollisionCircles(b->position, b->radius, e->position, e->radius)) continue;

            b->active = false;                          /* o tiro some ao acertar */
            Game_OnEnemyHit(g, e, b->damage, b->position);
            break;                                      /* um tiro acerta só um inimigo */
        }
    }

    /* 2) inimigos x jogador (bater na nave) */
    for (int j = 0; j < MAX_ENEMIES; j++) {
        Enemy *e = &g->enemies[j];
        if (!e->active) continue;
        if (!PlayerVulnerable(g)) break;

        if (CheckCollisionCircles(g->player.position, g->player.radius, e->position, e->radius)) {
            Game_OnPlayerHit(g);
            if (e->type != ENEMY_BOSS) Game_OnEnemyRammed(g, e);   /* o chefe não explode ao bater */
        }
    }

    /* 3) tiros inimigos x jogador */
    for (int i = 0; i < MAX_PROJECTILES; i++) {
        Projectile *b = &g->projectiles[i];
        if (!b->active || b->owner != OWNER_ENEMY) continue;
        if (!PlayerVulnerable(g)) break;

        if (CheckCollisionCircles(g->player.position, g->player.radius, b->position, b->radius)) {
            b->active = false;
            Game_OnPlayerHit(g);
        }
    }

    /* 4) power-ups x jogador */
    if (g->player.alive) {
        for (int i = 0; i < MAX_POWERUPS; i++) {
            PowerUp *p = &g->powerUps[i];
            if (!p->active) continue;

            if (CheckCollisionCircles(g->player.position, g->player.radius + POWERUP_PICKUP_BONUS,
                                      p->position, p->radius)) {
                Game_OnPowerUpCollected(g, p);
            }
        }
    }
}
