/*
 * powerup.h — Itens coletáveis que caem dos inimigos destruídos
 */
#ifndef POWERUP_H
#define POWERUP_H

#include <stdbool.h>
#include "raylib.h"

typedef enum {
    POWERUP_WEAPON,    /* "A": melhora a arma (até o nível 3) */
    POWERUP_SHIELD,    /* "E": escudo que absorve um golpe */
    POWERUP_LIFE,      /* "+": vida extra */
    POWERUP_TYPE_COUNT
} PowerUpType;

typedef struct PowerUp {
    bool active;
    PowerUpType type;
    Vector2 position;
    float radius;
    float age;
} PowerUp;

void  PowerUp_Spawn(PowerUp *list, int max, Vector2 position, PowerUpType type);
void  PowerUp_UpdateAll(PowerUp *list, int max, float dt);
void  PowerUp_DrawAll(const PowerUp *list, int max);
void  PowerUp_ClearAll(PowerUp *list, int max);
Color PowerUp_Color(PowerUpType type);

#endif /* POWERUP_H */
