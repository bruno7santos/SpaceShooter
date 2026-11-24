/*
 * powerup.c — Itens de bônus
 */
#include "powerup.h"

#include <math.h>
#include <string.h>
#include "config.h"
#include "gfx.h"
#include "utils.h"

#define POWERUP_RADIUS 17.0f
#define POWERUP_FALL   85.0f

Color PowerUp_Color(PowerUpType type)
{
    switch (type) {
        case POWERUP_WEAPON: return COL_ORANGE;
        case POWERUP_SHIELD: return COL_CYAN;
        case POWERUP_LIFE:   return COL_GREEN;
        default:             return COL_WHITE;
    }
}

void PowerUp_Spawn(PowerUp *list, int max, Vector2 position, PowerUpType type)
{
    for (int i = 0; i < max; i++) {
        if (list[i].active) continue;

        list[i].active = true;
        list[i].type = type;
        list[i].position = position;
        list[i].radius = POWERUP_RADIUS;
        list[i].age = 0.0f;
        return;
    }
    /* array cheio: o item simplesmente não aparece */
}

void PowerUp_UpdateAll(PowerUp *list, int max, float dt)
{
    for (int i = 0; i < max; i++) {
        PowerUp *p = &list[i];
        if (!p->active) continue;

        p->age += dt;
        p->position.y += POWERUP_FALL * dt;
        p->position.x += cosf(p->age * 2.2f) * 28.0f * dt;     /* balanço lateral */

        if (p->position.y - p->radius > (float)SCREEN_HEIGHT) p->active = false;
    }
}

void PowerUp_DrawAll(const PowerUp *list, int max)
{
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < max; i++) {
        const PowerUp *p = &list[i];
        if (p->active) Gfx_DrawGlow(p->position, p->radius * 2.8f, PowerUp_Color(p->type));
    }
    EndBlendMode();

    for (int i = 0; i < max; i++) {
        const PowerUp *p = &list[i];
        if (!p->active) continue;

        Color c = PowerUp_Color(p->type);
        float pulse = 1.0f + 0.08f * sinf(p->age * 8.0f);
        const char *label = (p->type == POWERUP_WEAPON) ? "A" : ((p->type == POWERUP_SHIELD) ? "E" : "+");

        DrawCircleV(p->position, p->radius * pulse, (Color){ 12, 16, 40, 235 });
        DrawRing(p->position, p->radius * pulse - 3.0f, p->radius * pulse, 0.0f, 360.0f, 28, c);
        Gfx_DrawTextCentered(label, p->position.x, p->position.y - 10.0f, 20.0f, COL_WHITE);
    }
}

void PowerUp_ClearAll(PowerUp *list, int max)
{
    memset(list, 0, (size_t)max * sizeof(PowerUp));
}
