/*
 * player.c — Movimento, tiro, dano e desenho da nave do jogador
 */
#include "player.h"

#include <math.h>
#include <string.h>
#include "audio.h"
#include "config.h"
#include "gfx.h"
#include "utils.h"

/* ------------------------------------------------------------------ */
/*  Formas da nave (coordenadas em pixels, relativas ao centro; y<0 = frente) */
/* ------------------------------------------------------------------ */

static const Vector2 HULL[7]    = { {0, -30}, {7, -12}, {10, 10}, {6, 22}, {-6, 22}, {-10, 10}, {-7, -12} };
static const Vector2 WING_R[4]  = { {9, 0}, {30, 22}, {30, 28}, {10, 22} };
static const Vector2 WING_L[4]  = { {-9, 0}, {-10, 22}, {-30, 28}, {-30, 22} };
static const Vector2 TIP_R[4]   = { {27, 12}, {31, 12}, {31, 25}, {27, 25} };
static const Vector2 TIP_L[4]   = { {-31, 12}, {-27, 12}, {-27, 25}, {-31, 25} };
static const Vector2 STRIPE[3]  = { {0, -22}, {3, -6}, {-3, -6} };

void Player_Init(Player *p)
{
    memset(p, 0, sizeof *p);
    p->position = (Vector2){ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT - 110.0f };
    p->radius = PLAYER_RADIUS;
    p->lives = PLAYER_START_LIVES;
    p->weaponLevel = 1;
    p->invulnTimer = 1.5f;        /* proteção curta ao começar */
    p->alive = true;
}

/* ------------------------------------------------------------------ */
/*  Tiro                                                               */
/* ------------------------------------------------------------------ */

static void Shoot(Player *p, Projectile *list, int max)
{
    const float noseY = p->position.y - 28.0f;
    const float x = p->position.x;

    switch (p->weaponLevel) {
        case 1:   /* tiro simples */
            Projectile_Spawn(list, max, (Vector2){ x, noseY }, (Vector2){ 0.0f, -BULLET_SPEED },
                             BULLET_RADIUS, 1, OWNER_PLAYER, COL_CYAN);
            p->fireCooldown = 0.20f;
            break;

        case 2:   /* tiro duplo, paralelo */
            Projectile_Spawn(list, max, (Vector2){ x - 12.0f, noseY + 6.0f }, (Vector2){ 0.0f, -BULLET_SPEED },
                             BULLET_RADIUS, 1, OWNER_PLAYER, COL_CYAN);
            Projectile_Spawn(list, max, (Vector2){ x + 12.0f, noseY + 6.0f }, (Vector2){ 0.0f, -BULLET_SPEED },
                             BULLET_RADIUS, 1, OWNER_PLAYER, COL_CYAN);
            p->fireCooldown = 0.18f;
            break;

        default:  /* tiro triplo: um reto e dois inclinados */
        {
            const float angle = 0.20f;   /* radianos (~11,5 graus) */
            Projectile_Spawn(list, max, (Vector2){ x, noseY }, (Vector2){ 0.0f, -BULLET_SPEED },
                             BULLET_RADIUS, 1, OWNER_PLAYER, COL_WHITE_SOFT);
            Projectile_Spawn(list, max, (Vector2){ x - 14.0f, noseY + 8.0f },
                             (Vector2){ -sinf(angle) * BULLET_SPEED, -cosf(angle) * BULLET_SPEED },
                             BULLET_RADIUS, 1, OWNER_PLAYER, COL_CYAN);
            Projectile_Spawn(list, max, (Vector2){ x + 14.0f, noseY + 8.0f },
                             (Vector2){ sinf(angle) * BULLET_SPEED, -cosf(angle) * BULLET_SPEED },
                             BULLET_RADIUS, 1, OWNER_PLAYER, COL_CYAN);
            p->fireCooldown = 0.16f;
            break;
        }
    }
    Audio_Play(SFX_SHOOT);
}

/* ------------------------------------------------------------------ */
/*  Atualização                                                        */
/* ------------------------------------------------------------------ */

void Player_Update(Player *p, float dt, Projectile *projectiles, int maxProjectiles)
{
    /* --- entrada: WASD ou setas --- */
    Vector2 dir = { 0.0f, 0.0f };
    if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) dir.x -= 1.0f;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) dir.x += 1.0f;
    if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) dir.y -= 1.0f;
    if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) dir.y += 1.0f;

    /* normaliza para que andar na diagonal não seja mais rápido */
    if (dir.x != 0.0f || dir.y != 0.0f) dir = Vector2Normalize(dir);

    /* aceleração suave, independente do FPS: aproxima a velocidade atual da desejada */
    Vector2 target = Vector2Scale(dir, PLAYER_SPEED);
    float blend = 1.0f - expf(-PLAYER_ACCEL * dt);
    p->velocity = Vector2Lerp(p->velocity, target, blend);
    p->position = Vector2Add(p->position, Vector2Scale(p->velocity, dt));

    /* mantém a nave dentro da tela */
    if (p->position.x < PLAYER_MARGIN)                  { p->position.x = PLAYER_MARGIN;                  p->velocity.x = 0.0f; }
    if (p->position.x > SCREEN_WIDTH - PLAYER_MARGIN)   { p->position.x = SCREEN_WIDTH - PLAYER_MARGIN;  p->velocity.x = 0.0f; }
    if (p->position.y < PLAYER_MARGIN)                  { p->position.y = PLAYER_MARGIN;                  p->velocity.y = 0.0f; }
    if (p->position.y > SCREEN_HEIGHT - PLAYER_MARGIN)  { p->position.y = SCREEN_HEIGHT - PLAYER_MARGIN; p->velocity.y = 0.0f; }

    /* --- temporizadores --- */
    if (p->invulnTimer > 0.0f) p->invulnTimer -= dt;
    if (p->shieldTimer > 0.0f) p->shieldTimer -= dt;
    if (p->fireCooldown > 0.0f) p->fireCooldown -= dt;
    p->animTime += dt;

    /* --- tiro: segurar ESPAÇO dispara automaticamente --- */
    if (IsKeyDown(KEY_SPACE) && p->fireCooldown <= 0.0f) {
        Shoot(p, projectiles, maxProjectiles);
    }
}

/* ------------------------------------------------------------------ */
/*  Dano e power-ups                                                   */
/* ------------------------------------------------------------------ */

bool Player_CanBeHit(const Player *p)
{
    return p->alive && p->invulnTimer <= 0.0f;
}

HitResult Player_TakeDamage(Player *p)
{
    if (!Player_CanBeHit(p)) return HIT_IGNORED;

    if (p->shieldTimer > 0.0f) {          /* o escudo absorve o golpe e some */
        p->shieldTimer = 0.0f;
        p->invulnTimer = 0.6f;
        return HIT_SHIELD_BLOCKED;
    }

    p->lives--;
    p->invulnTimer = PLAYER_INVULN_TIME;  /* evita perder todas as vidas de uma vez */
    if (p->weaponLevel > 1) p->weaponLevel--;
    return HIT_LIFE_LOST;
}

bool Player_AddLife(Player *p)
{
    if (p->lives >= PLAYER_MAX_LIVES) return false;
    p->lives++;
    return true;
}

bool Player_UpgradeWeapon(Player *p)
{
    if (p->weaponLevel >= PLAYER_MAX_WEAPON) return false;
    p->weaponLevel++;
    return true;
}

void Player_GiveShield(Player *p)
{
    p->shieldTimer = PLAYER_SHIELD_TIME;
}

/* ------------------------------------------------------------------ */
/*  Desenho                                                            */
/* ------------------------------------------------------------------ */

void Player_DrawShip(Vector2 pos, float scale, float alpha, bool flame, float time)
{
    const Color hull   = ColorWithAlpha((Color){ 214, 224, 244, 255 }, alpha);
    const Color wing   = ColorWithAlpha((Color){  52, 118, 232, 255 }, alpha);
    const Color edge   = ColorWithAlpha((Color){  16,  44, 120, 255 }, alpha);
    const Color accent = ColorWithAlpha(COL_ORANGE, alpha);

    /* chama do motor (blend aditivo) */
    if (flame) {
        float flicker = 0.5f + 0.5f * sinf(time * 55.0f) * cosf(time * 23.0f);
        float len = 14.0f + 10.0f * flicker;
        Vector2 outer[3] = { {-6, 22}, {6, 22}, {0, 22 + len} };
        Vector2 inner[3] = { {-3, 22}, {3, 22}, {0, 22 + len * 0.55f} };

        BeginBlendMode(BLEND_ADDITIVE);
        Gfx_FillPolygon(outer, 3, pos, scale, ColorWithAlpha(COL_ORANGE, 0.85f * alpha));
        Gfx_FillPolygon(inner, 3, pos, scale, ColorWithAlpha(COL_GOLD, 0.95f * alpha));
        EndBlendMode();
    }

    /* asas */
    Gfx_FillPolygon(WING_R, 4, pos, scale, wing);
    Gfx_FillPolygon(WING_L, 4, pos, scale, wing);
    Gfx_OutlinePolygon(WING_R, 4, pos, scale, 1.5f * scale, edge);
    Gfx_OutlinePolygon(WING_L, 4, pos, scale, 1.5f * scale, edge);
    Gfx_FillPolygon(TIP_R, 4, pos, scale, accent);
    Gfx_FillPolygon(TIP_L, 4, pos, scale, accent);

    /* fuselagem */
    Gfx_FillPolygon(HULL, 7, pos, scale, hull);
    Gfx_OutlinePolygon(HULL, 7, pos, scale, 1.5f * scale, edge);
    Gfx_FillPolygon(STRIPE, 3, pos, scale, wing);

    /* cabine */
    Gfx_FillEllipse((Vector2){ pos.x, pos.y - 4.0f * scale }, 4.5f * scale, 9.0f * scale,
                    ColorWithAlpha((Color){ 110, 225, 255, 255 }, alpha));
    Gfx_FillEllipse((Vector2){ pos.x - 1.2f * scale, pos.y - 7.0f * scale }, 1.6f * scale, 3.5f * scale,
                    ColorWithAlpha(COL_WHITE, 0.85f * alpha));
}

void Player_Draw(const Player *p)
{
    if (!p->alive) return;

    /* durante a invulnerabilidade a nave pisca */
    float alpha = 1.0f;
    if (p->invulnTimer > 0.0f) alpha = BlinkOn(p->invulnTimer, 0.2f) ? 0.25f : 1.0f;

    Player_DrawShip(p->position, 1.0f, alpha, true, p->animTime);

    /* escudo: anel pulsante (pisca nos últimos 2 segundos) */
    if (p->shieldTimer > 0.0f) {
        bool expiring = p->shieldTimer < 2.0f;
        if (!expiring || BlinkOn(p->shieldTimer, 0.3f)) {
            float pulse = 0.5f + 0.5f * sinf(p->animTime * 7.0f);
            float r = 40.0f + 2.0f * pulse;
            DrawCircleV(p->position, r, ColorWithAlpha(COL_CYAN, 0.10f + 0.05f * pulse));
            DrawRing(p->position, r - 3.0f, r, 0.0f, 360.0f, 36, ColorWithAlpha(COL_CYAN, 0.75f));
        }
    }
}
