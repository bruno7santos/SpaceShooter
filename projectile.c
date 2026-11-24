/*
 * projectile.c — Movimento, desenho e gerenciamento dos tiros
 */
#include "projectile.h"

#include <string.h>
#include "config.h"
#include "gfx.h"
#include "utils.h"

#define OFFSCREEN_MARGIN 60.0f

bool Projectile_Spawn(Projectile *list, int max, Vector2 position, Vector2 velocity,
                      float radius, int damage, ProjectileOwner owner, Color color)
{
    for (int i = 0; i < max; i++) {
        if (list[i].active) continue;

        list[i].active = true;
        list[i].owner = owner;
        list[i].position = position;
        list[i].velocity = velocity;
        list[i].radius = radius;
        list[i].damage = damage;
        list[i].color = color;
        return true;
    }
    return false;   /* sem espaço: ignora o tiro em vez de estourar o array */
}

void Projectile_UpdateAll(Projectile *list, int max, float dt)
{
    for (int i = 0; i < max; i++) {
        Projectile *p = &list[i];
        if (!p->active) continue;

        p->position = Vector2Add(p->position, Vector2Scale(p->velocity, dt));

        /* saiu da tela: libera o espaço no array */
        if (p->position.x < -OFFSCREEN_MARGIN || p->position.x > SCREEN_WIDTH + OFFSCREEN_MARGIN ||
            p->position.y < -OFFSCREEN_MARGIN || p->position.y > SCREEN_HEIGHT + OFFSCREEN_MARGIN) {
            p->active = false;
        }
    }
}

void Projectile_DrawAll(const Projectile *list, int max)
{
    /* 1) brilho (blend aditivo) */
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < max; i++) {
        const Projectile *p = &list[i];
        if (p->active) Gfx_DrawGlow(p->position, p->radius * 3.4f, p->color);
    }
    EndBlendMode();

    /* 2) corpo do tiro */
    for (int i = 0; i < max; i++) {
        const Projectile *p = &list[i];
        if (!p->active) continue;

        if (p->owner == OWNER_PLAYER) {
            /* raio alongado na direção do movimento */
            Vector2 dir = Vector2Normalize(p->velocity);
            Vector2 tail = Vector2Subtract(p->position, Vector2Scale(dir, 16.0f));
            Vector2 head = Vector2Add(p->position, Vector2Scale(dir, 8.0f));
            DrawLineEx(tail, head, p->radius * 1.3f, p->color);
            DrawLineEx(Vector2Subtract(p->position, Vector2Scale(dir, 9.0f)), head, p->radius * 0.6f, COL_WHITE);
        } else {
            DrawCircleV(p->position, p->radius, p->color);
            DrawCircleV(p->position, p->radius * 0.5f, COL_WHITE);
        }
    }
}

void Projectile_ClearAll(Projectile *list, int max)
{
    memset(list, 0, (size_t)max * sizeof(Projectile));
}

void Projectile_ClearOwner(Projectile *list, int max, ProjectileOwner owner)
{
    for (int i = 0; i < max; i++) {
        if (list[i].owner == owner) list[i].active = false;
    }
}
