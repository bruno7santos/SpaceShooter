/*
 * enemy.c — Comportamento (IA), colisão e desenho dos inimigos
 * Cada tipo tem uma função de atualização própria; o switch em
 * Enemy_UpdateAll escolhe qual usar. Os sprites são desenhados com
 * polígonos e formas simples da raylib (nenhuma imagem externa).
 */
#include "enemy.h"

#include <math.h>
#include <string.h>
#include "audio.h"
#include "config.h"
#include "gfx.h"
#include "utils.h"

/* ------------------------------------------------------------------ */
/*  Parâmetros de cada tipo                                            */
/* ------------------------------------------------------------------ */

typedef struct {
    float radius;
    int   hp;
    int   points;
    float speed;     /* velocidade vertical básica (px/s) */
} EnemyStats;

static const EnemyStats STATS[ENEMY_TYPE_COUNT] = {
    /* SCOUT    */ { 18.0f,    1,  100, 150.0f },
    /* ZIGZAG   */ { 19.0f,    1,  150, 125.0f },
    /* KAMIKAZE */ { 16.0f,    1,  200, 190.0f },
    /* TANK     */ { 30.0f,    6,  300,  70.0f },
    /* SHOOTER  */ { 23.0f,    3,  250, 150.0f },
    /* BOSS     */ { 74.0f,  150, 5000,  80.0f }
};

#define ZIGZAG_AMPLITUDE     150.0f
#define ZIGZAG_FREQUENCY     2.2f
#define KAMIKAZE_SIDE_SPEED  170.0f
#define SHOOTER_HOVER_TIME   7.0f
#define BOSS_HOVER_Y         150.0f
#define BOSS_PATROL_RANGE    430.0f
#define ENEMY_EDGE_MARGIN    30.0f

#define ENEMY_BULLET_COLOR   ((Color){ 255, 110,  60, 255 })
#define BOSS_BULLET_COLOR    ((Color){ 255,  80, 200, 255 })

Color Enemy_Color(EnemyType type)
{
    switch (type) {
        case ENEMY_SCOUT:    return COL_RED;
        case ENEMY_ZIGZAG:   return COL_PURPLE;
        case ENEMY_KAMIKAZE: return COL_ORANGE;
        case ENEMY_TANK:     return COL_BLUE;
        case ENEMY_SHOOTER:  return COL_GREEN;
        case ENEMY_BOSS:     return COL_MAGENTA;
        default:             return COL_WHITE;
    }
}

/* ------------------------------------------------------------------ */
/*  Criação e utilidades                                               */
/* ------------------------------------------------------------------ */

Enemy *Enemy_Spawn(Enemy *list, int max, EnemyType type, float x, float y, float difficulty)
{
    int t = (int)type;
    if (t < 0 || t >= ENEMY_TYPE_COUNT) return NULL;

    for (int i = 0; i < max; i++) {
        Enemy *e = &list[i];
        if (e->active) continue;

        const EnemyStats *st = &STATS[t];
        memset(e, 0, sizeof *e);

        e->active = true;
        e->type = type;
        e->radius = st->radius;
        e->points = st->points;
        e->hp = st->hp;
        if (type == ENEMY_TANK || type == ENEMY_SHOOTER) e->hp += (int)(difficulty / 2.0f);
        e->maxHp = e->hp;
        e->speedScale = 1.0f + DIFFICULTY_SPEED_STEP * difficulty;
        e->velocity = (Vector2){ 0.0f, st->speed * e->speedScale * RandFloat(0.9f, 1.15f) };
        e->phase = RandFloat(0.0f, 2.0f * PI);
        e->shootTimer = RandFloat(0.8f, 1.6f);
        e->hoverY = RandFloat(110.0f, 260.0f);
        e->position = (Vector2){ x, y };
        e->baseX = x;

        if (type == ENEMY_ZIGZAG) {
            /* escolhe um eixo central que deixa a onda inteira dentro da tela */
            e->baseX = Clamp(x, ZIGZAG_AMPLITUDE + ENEMY_EDGE_MARGIN, SCREEN_WIDTH - ZIGZAG_AMPLITUDE - ENEMY_EDGE_MARGIN);
            e->position.x = e->baseX + sinf(e->phase) * ZIGZAG_AMPLITUDE;
        } else if (type == ENEMY_TANK) {
            e->baseX = Clamp(x, 80.0f, SCREEN_WIDTH - 80.0f);
            e->position.x = e->baseX + sinf(e->phase) * 40.0f;
        } else if (type == ENEMY_BOSS) {
            e->baseX = SCREEN_WIDTH * 0.5f;
            e->position.x = e->baseX;
            e->phase = 0.0f;
            e->velocity = (Vector2){ 0.0f, 0.0f };
        }
        return e;
    }
    return NULL;   /* array cheio */
}

void Enemy_ClearAll(Enemy *list, int max)
{
    memset(list, 0, (size_t)max * sizeof(Enemy));
}

bool Enemy_Hit(Enemy *e, int damage)
{
    e->hp -= damage;
    e->hitFlash = 0.09f;
    if (e->hp <= 0) {
        e->active = false;
        return true;
    }
    return false;
}

bool Enemy_IsVulnerable(const Enemy *e)
{
    return !(e->type == ENEMY_BOSS && e->state == 0);   /* state 0 = chefe ainda entrando */
}

int Enemy_CountActive(const Enemy *list, int max)
{
    int count = 0;
    for (int i = 0; i < max; i++) {
        if (list[i].active) count++;
    }
    return count;
}

const Enemy *Enemy_FindBoss(const Enemy *list, int max)
{
    for (int i = 0; i < max; i++) {
        if (list[i].active && list[i].type == ENEMY_BOSS) return &list[i];
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/*  Disparos                                                           */
/* ------------------------------------------------------------------ */

/* Atira na direção do jogador, com um desvio opcional de "angleOffset" radianos. */
static void FireAimed(Vector2 origin, const EnemyContext *ctx, float speed, float angleOffset, Color color)
{
    Vector2 dir = Vector2Subtract(ctx->targetPos, origin);
    if (Vector2Length(dir) < 1.0f) dir = (Vector2){ 0.0f, 1.0f };
    dir = Vector2Rotate(Vector2Normalize(dir), angleOffset);

    Projectile_Spawn(ctx->projectiles, ctx->maxProjectiles, origin, Vector2Scale(dir, speed),
                     ENEMY_BULLET_RADIUS, 1, OWNER_ENEMY, color);
}

/* Rajada circular: "count" tiros espalhados em 360 graus. */
static void FireRadial(Vector2 origin, const EnemyContext *ctx, int count, float speed, float startAngle, Color color)
{
    for (int i = 0; i < count; i++) {
        float a = startAngle + (2.0f * PI * (float)i) / (float)count;
        Projectile_Spawn(ctx->projectiles, ctx->maxProjectiles, origin,
                         (Vector2){ cosf(a) * speed, sinf(a) * speed },
                         ENEMY_BULLET_RADIUS, 1, OWNER_ENEMY, color);
    }
}

/* ------------------------------------------------------------------ */
/*  Atualização (IA) de cada tipo                                      */
/* ------------------------------------------------------------------ */

static void UpdateScout(Enemy *e, float dt)
{
    e->position.y += e->velocity.y * dt;               /* desce em linha reta */
}

static void UpdateZigzag(Enemy *e, float dt)
{
    e->position.y += e->velocity.y * dt;
    e->position.x = e->baseX + sinf(e->age * ZIGZAG_FREQUENCY + e->phase) * ZIGZAG_AMPLITUDE;
}

static void UpdateKamikaze(Enemy *e, float dt, const EnemyContext *ctx)
{
    /* enquanto está acima do jogador, ajusta a posição horizontal na direção dele */
    float desired = 0.0f;
    if (e->position.y < ctx->targetPos.y - 60.0f) {
        desired = Clamp((ctx->targetPos.x - e->position.x) * 1.6f, -KAMIKAZE_SIDE_SPEED, KAMIKAZE_SIDE_SPEED);
    }
    e->velocity.x = Lerp(e->velocity.x, desired, 1.0f - expf(-3.0f * dt));
    e->position.x += e->velocity.x * dt;
    e->position.y += e->velocity.y * dt;
}

static void UpdateTank(Enemy *e, float dt)
{
    e->position.y += e->velocity.y * dt;
    e->position.x = e->baseX + sinf(e->age * 0.9f + e->phase) * 40.0f;   /* balanço lento */
}

static void UpdateShooter(Enemy *e, float dt, const EnemyContext *ctx)
{
    switch (e->state) {
        case 0:   /* desce até a altura de patrulha */
            e->position.y += e->velocity.y * 1.5f * dt;
            if (e->position.y >= e->hoverY) {
                e->state = 1;
                e->stateTimer = SHOOTER_HOVER_TIME;
                e->velocity.x = RandSign() * 95.0f;
            }
            break;

        case 1:   /* patrulha de um lado a outro, atirando */
            e->position.x += e->velocity.x * dt;
            if (e->position.x < ENEMY_EDGE_MARGIN) {
                e->position.x = ENEMY_EDGE_MARGIN;
                e->velocity.x = fabsf(e->velocity.x);
            }
            if (e->position.x > SCREEN_WIDTH - ENEMY_EDGE_MARGIN) {
                e->position.x = SCREEN_WIDTH - ENEMY_EDGE_MARGIN;
                e->velocity.x = -fabsf(e->velocity.x);
            }

            e->shootTimer -= dt;
            if (e->shootTimer <= 0.0f && ctx->targetPos.y > e->position.y) {
                FireAimed(e->position, ctx, ENEMY_BULLET_SPEED, 0.0f, ENEMY_BULLET_COLOR);
                Audio_Play(SFX_ENEMY_SHOOT);
                e->shootTimer = 2.0f / e->speedScale;
            }

            e->stateTimer -= dt;
            if (e->stateTimer <= 0.0f) e->state = 2;
            break;

        default:  /* vai embora pela parte de baixo */
            e->position.y += 230.0f * dt;
            break;
    }
}

/* Fase do chefe conforme a vida restante: 1 (inicial), 2 (abaixo de 66%) e 3 (abaixo de 33%). */
int Enemy_BossPhase(const Enemy *e)
{
    float ratio = (float)e->hp / (float)e->maxHp;
    if (ratio > 0.66f) return 1;
    if (ratio > 0.33f) return 2;
    return 3;
}

static void UpdateBoss(Enemy *e, float dt, const EnemyContext *ctx)
{
    if (e->state == 0) {                          /* entrando na tela (invulnerável) */
        e->position.y += 75.0f * dt;
        if (e->position.y >= BOSS_HOVER_Y) {
            e->position.y = BOSS_HOVER_Y;
            e->state = 1;
            e->shootTimer = 1.2f;
            e->burstTimer = 2.5f;
        }
        return;
    }

    int phase = Enemy_BossPhase(e);

    /* patrulha horizontal: o ângulo é acumulado para o movimento não "pular" quando a fase muda */
    e->phase += (0.45f + 0.12f * (float)phase) * dt;
    e->position.x = SCREEN_WIDTH * 0.5f + sinf(e->phase) * BOSS_PATROL_RANGE;
    e->position.y = BOSS_HOVER_Y + sinf(e->age * 1.7f) * 14.0f;

    /* leque de tiros mirados no jogador */
    e->shootTimer -= dt;
    if (e->shootTimer <= 0.0f) {
        Vector2 muzzle = { e->position.x, e->position.y + e->radius * 0.8f };
        int shots = (phase == 3) ? 5 : 3;
        for (int k = 0; k < shots; k++) {
            float offset = ((float)k - (float)(shots - 1) * 0.5f) * 0.2f;
            FireAimed(muzzle, ctx, 330.0f, offset, BOSS_BULLET_COLOR);
        }
        Audio_Play(SFX_ENEMY_SHOOT);
        e->shootTimer = (phase == 1) ? 1.5f : ((phase == 2) ? 1.25f : 1.0f);
    }

    /* a partir da fase 2: rajada circular */
    if (phase >= 2) {
        e->burstTimer -= dt;
        if (e->burstTimer <= 0.0f) {
            FireRadial(e->position, ctx, (phase == 3) ? 22 : 16, 215.0f, RandFloat(0.0f, 2.0f * PI), BOSS_BULLET_COLOR);
            Audio_Play(SFX_ENEMY_SHOOT);
            e->burstTimer = (phase == 3) ? 2.4f : 3.2f;
        }
    }
}

void Enemy_UpdateAll(Enemy *list, int max, float dt, const EnemyContext *ctx)
{
    for (int i = 0; i < max; i++) {
        Enemy *e = &list[i];
        if (!e->active) continue;

        e->age += dt;
        if (e->hitFlash > 0.0f) e->hitFlash -= dt;

        switch (e->type) {
            case ENEMY_SCOUT:    UpdateScout(e, dt);            break;
            case ENEMY_ZIGZAG:   UpdateZigzag(e, dt);           break;
            case ENEMY_KAMIKAZE: UpdateKamikaze(e, dt, ctx);    break;
            case ENEMY_TANK:     UpdateTank(e, dt);             break;
            case ENEMY_SHOOTER:  UpdateShooter(e, dt, ctx);     break;
            case ENEMY_BOSS:     UpdateBoss(e, dt, ctx);        break;
            default: break;
        }

        /* saiu pela parte de baixo: o inimigo "escapou" e libera o espaço no array */
        if (e->position.y - e->radius > (float)SCREEN_HEIGHT + 20.0f) e->active = false;
    }
}

/* ------------------------------------------------------------------ */
/*  Desenho (formas normalizadas: multiplicadas pelo raio do inimigo)  */
/* ------------------------------------------------------------------ */

static const Vector2 SCOUT_WING_R[3] = { { 0.0f, 1.1f }, { 1.0f, -0.8f }, { 0.0f, -0.3f } };
static const Vector2 SCOUT_WING_L[3] = { { 0.0f, 1.1f }, { 0.0f, -0.3f }, { -1.0f, -0.8f } };
static const Vector2 SCOUT_SPINE[4]  = { { 0.0f, 0.95f }, { 0.28f, 0.05f }, { 0.0f, -0.5f }, { -0.28f, 0.05f } };

static const Vector2 KAMI_BODY[5]    = { { 0.0f, 1.1f }, { 0.7f, -0.2f }, { 0.35f, -1.0f }, { -0.35f, -1.0f }, { -0.7f, -0.2f } };
static const Vector2 KAMI_STRIPE[3]  = { { 0.0f, 0.9f }, { 0.2f, -0.4f }, { -0.2f, -0.4f } };

static const Vector2 TANK_GUN_L[4]   = { { -0.62f, 0.5f }, { -0.38f, 0.5f }, { -0.38f, 1.25f }, { -0.62f, 1.25f } };
static const Vector2 TANK_GUN_R[4]   = { { 0.38f, 0.5f }, { 0.62f, 0.5f }, { 0.62f, 1.25f }, { 0.38f, 1.25f } };

static const Vector2 SHOOTER_BODY[4] = { { -1.0f, -0.7f }, { 1.0f, -0.7f }, { 0.65f, 0.55f }, { -0.65f, 0.55f } };
static const Vector2 SHOOTER_GUN_L[4]= { { -0.62f, 0.45f }, { -0.38f, 0.45f }, { -0.38f, 1.2f }, { -0.62f, 1.2f } };
static const Vector2 SHOOTER_GUN_R[4]= { { 0.38f, 0.45f }, { 0.62f, 0.45f }, { 0.62f, 1.2f }, { 0.38f, 1.2f } };

static const Vector2 BOSS_WING_R[4]  = { { 0.55f, -0.55f }, { 1.25f, -0.25f }, { 1.20f, 0.45f }, { 0.55f, 0.30f } };
static const Vector2 BOSS_WING_L[4]  = { { -0.55f, -0.55f }, { -0.55f, 0.30f }, { -1.20f, 0.45f }, { -1.25f, -0.25f } };
static const Vector2 BOSS_GUN[4]     = { { -0.09f, 0.6f }, { 0.09f, 0.6f }, { 0.09f, 1.12f }, { -0.09f, 1.12f } };

static void DrawScout(const Enemy *e, float flash)
{
    Vector2 p = e->position;
    float r = e->radius;
    Color wingA = ColorMix((Color){ 230,  60,  75, 255 }, COL_WHITE, flash);
    Color wingB = ColorMix((Color){ 170,  35,  55, 255 }, COL_WHITE, flash);
    Color spine = ColorMix((Color){ 255, 150, 150, 255 }, COL_WHITE, flash);
    Color edge  = (Color){ 90, 10, 30, 255 };

    Gfx_FillPolygon(SCOUT_WING_R, 3, p, r, wingA);
    Gfx_FillPolygon(SCOUT_WING_L, 3, p, r, wingB);
    Gfx_FillPolygon(SCOUT_SPINE, 4, p, r, spine);
    Gfx_OutlinePolygon(SCOUT_WING_R, 3, p, r, 1.5f, edge);
    Gfx_OutlinePolygon(SCOUT_WING_L, 3, p, r, 1.5f, edge);
    DrawCircleV((Vector2){ p.x, p.y + r * 0.1f }, r * 0.18f, (Color){ 40, 10, 30, 255 });
}

static void DrawZigzag(const Enemy *e, float flash)
{
    Vector2 p = e->position;
    float r = e->radius;
    Color dome = ColorMix((Color){ 205, 175, 255, 255 }, COL_WHITE, flash);
    Color disc = ColorMix((Color){ 150,  80, 230, 255 }, COL_WHITE, flash);
    Color rim  = ColorMix((Color){  90,  40, 160, 255 }, COL_WHITE, flash);

    Gfx_FillEllipse((Vector2){ p.x, p.y - r * 0.25f }, r * 0.55f, r * 0.50f, dome);      /* cúpula */
    Gfx_FillEllipse((Vector2){ p.x, p.y + r * 0.10f }, r * 1.15f, r * 0.48f, disc);      /* disco */
    Gfx_FillEllipse((Vector2){ p.x, p.y + r * 0.24f }, r * 1.00f, r * 0.24f, rim);       /* sombra */

    for (int i = 0; i < 3; i++) {                                                        /* luzes piscando */
        bool on = fmodf(e->age * 4.0f + (float)i, 3.0f) < 1.0f;
        Vector2 lamp = { p.x + ((float)i - 1.0f) * r * 0.6f, p.y + r * (i == 1 ? 0.20f : 0.08f) };
        DrawCircleV(lamp, r * 0.11f, on ? COL_GOLD : (Color){ 70, 40, 110, 255 });
    }
}

static void DrawKamikaze(const Enemy *e, float flash)
{
    Vector2 p = e->position;
    float r = e->radius;
    Color body   = ColorMix((Color){ 255, 150,  40, 255 }, COL_WHITE, flash);
    Color stripe = ColorMix((Color){ 255, 235, 120, 255 }, COL_WHITE, flash);

    /* chama (o inimigo vai "de ponta-cabeça": a traseira fica em cima) */
    float flicker = 0.5f + 0.5f * sinf(e->age * 45.0f);
    Vector2 flame[3] = { { -0.3f, -1.0f }, { 0.3f, -1.0f }, { 0.0f, -1.7f - 0.5f * flicker } };
    BeginBlendMode(BLEND_ADDITIVE);
    Gfx_FillPolygon(flame, 3, p, r, ColorWithAlpha(COL_ORANGE, 0.9f));
    EndBlendMode();

    Gfx_FillPolygon(KAMI_BODY, 5, p, r, body);
    Gfx_OutlinePolygon(KAMI_BODY, 5, p, r, 1.5f, (Color){ 120, 50, 10, 255 });
    Gfx_FillPolygon(KAMI_STRIPE, 3, p, r, stripe);
}

static void DrawTank(const Enemy *e, float flash)
{
    Vector2 p = e->position;
    float r = e->radius;
    Color outer = ColorMix((Color){  96, 112, 146, 255 }, COL_WHITE, flash);
    Color inner = ColorMix((Color){  44,  54,  84, 255 }, COL_WHITE, flash);
    Color gun   = ColorMix((Color){  60,  70, 100, 255 }, COL_WHITE, flash);

    Gfx_FillPolygon(TANK_GUN_L, 4, p, r, gun);
    Gfx_FillPolygon(TANK_GUN_R, 4, p, r, gun);

    Gfx_FillRegular(p, 6, r, 0.0f, outer);
    Gfx_FillRegular(p, 6, r * 0.72f, 0.0f, inner);
    Gfx_OutlineRegular(p, 6, r, 0.0f, 2.0f, (Color){ 20, 26, 50, 255 });

    float pulse = 0.5f + 0.5f * sinf(e->age * 5.0f);                 /* núcleo pulsante */
    DrawCircleV(p, r * 0.30f, ColorMix(COL_RED, COL_GOLD, pulse));
    DrawCircleV(p, r * 0.14f, COL_WHITE);
}

static void DrawShooter(const Enemy *e, float flash)
{
    Vector2 p = e->position;
    float r = e->radius;
    Color body = ColorMix((Color){  50, 170,  95, 255 }, COL_WHITE, flash);
    Color dark = ColorMix((Color){  25,  90,  55, 255 }, COL_WHITE, flash);

    Gfx_FillPolygon(SHOOTER_GUN_L, 4, p, r, dark);
    Gfx_FillPolygon(SHOOTER_GUN_R, 4, p, r, dark);
    Gfx_FillPolygon(SHOOTER_BODY, 4, p, r, body);
    Gfx_OutlinePolygon(SHOOTER_BODY, 4, p, r, 1.5f, (Color){ 10, 50, 30, 255 });

    DrawCircleV((Vector2){ p.x, p.y - r * 0.05f }, r * 0.32f, COL_GOLD);                 /* olho */
    DrawCircleV((Vector2){ p.x, p.y - r * 0.05f + r * 0.08f }, r * 0.14f, (Color){ 30, 20, 10, 255 });
}

static void DrawBoss(const Enemy *e, float flash)
{
    Vector2 p = e->position;
    float r = e->radius;
    float ratio = (float)e->hp / (float)e->maxHp;
    Color eye   = (ratio > 0.66f) ? COL_GOLD : ((ratio > 0.33f) ? COL_ORANGE : COL_RED);   /* muda com a fase */
    Color body  = ColorMix((Color){ 120,  28,  70, 255 }, COL_WHITE, flash);
    Color armor = ColorMix((Color){  70,  20,  50, 255 }, COL_WHITE, flash);
    Color trim  = ColorMix(COL_GOLD, COL_WHITE, flash);

    /* asas e canhões */
    Gfx_FillPolygon(BOSS_WING_R, 4, p, r, armor);
    Gfx_FillPolygon(BOSS_WING_L, 4, p, r, armor);
    Gfx_OutlinePolygon(BOSS_WING_R, 4, p, r, 2.0f, trim);
    Gfx_OutlinePolygon(BOSS_WING_L, 4, p, r, 2.0f, trim);
    for (int i = -1; i <= 1; i++) {
        Gfx_FillPolygon(BOSS_GUN, 4, (Vector2){ p.x + (float)i * r * 0.45f, p.y }, r, armor);
    }

    /* corpo: octógonos concêntricos */
    Gfx_FillRegular(p, 8, r * 0.85f, 22.5f, body);
    Gfx_OutlineRegular(p, 8, r * 0.85f, 22.5f, 4.0f, trim);
    Gfx_FillRegular(p, 8, r * 0.60f, 22.5f, armor);

    /* olho central pulsante */
    float pulse = 0.75f + 0.25f * sinf(e->age * 6.0f);
    BeginBlendMode(BLEND_ADDITIVE);
    Gfx_DrawGlow(p, r * 0.55f * pulse, eye);
    EndBlendMode();
    DrawCircleV(p, r * 0.26f, ColorMix(eye, COL_WHITE, flash));
    Gfx_FillEllipse(p, r * 0.07f, r * 0.20f, COL_BLACK);
}

static void DrawEnemy(const Enemy *e)
{
    /* o chefe leva tiros o tempo todo: o flash dele é bem mais discreto que o dos outros inimigos */
    float flash = (e->hitFlash > 0.0f) ? ((e->type == ENEMY_BOSS) ? 0.30f : 0.80f) : 0.0f;

    switch (e->type) {
        case ENEMY_SCOUT:    DrawScout(e, flash);    break;
        case ENEMY_ZIGZAG:   DrawZigzag(e, flash);   break;
        case ENEMY_KAMIKAZE: DrawKamikaze(e, flash); break;
        case ENEMY_TANK:     DrawTank(e, flash);     break;
        case ENEMY_SHOOTER:  DrawShooter(e, flash);  break;
        case ENEMY_BOSS:     DrawBoss(e, flash);     break;
        default: break;
    }
}

void Enemy_DrawAll(const Enemy *list, int max)
{
    for (int i = 0; i < max; i++) {
        const Enemy *e = &list[i];
        if (!e->active) continue;

        DrawEnemy(e);

        /* barrinha de vida para inimigos resistentes (o chefe tem a sua no HUD) */
        if (e->type != ENEMY_BOSS && e->maxHp > 1 && e->hp < e->maxHp) {
            Rectangle bar = { e->position.x - e->radius, e->position.y - e->radius - 12.0f, e->radius * 2.0f, 5.0f };
            Gfx_DrawBar(bar, (float)e->hp / (float)e->maxHp, COL_GREEN, ColorWithAlpha(COL_BLACK, 0.6f));
        }
    }
}

void Enemy_DrawPreview(EnemyType type, Vector2 position, float scale, float time)
{
    int t = (int)type;
    if (t < 0 || t >= ENEMY_TYPE_COUNT) return;

    Enemy e;
    memset(&e, 0, sizeof e);
    e.active = true;
    e.type = type;
    e.position = position;
    e.radius = STATS[t].radius * scale;
    e.hp = e.maxHp = 1;
    e.age = time;
    DrawEnemy(&e);
}
