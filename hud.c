/*
 * hud.c — Interface durante a partida
 */
#include "hud.h"

#include <stddef.h>
#include <math.h>
#include "config.h"
#include "enemy.h"
#include "gfx.h"
#include "player.h"
#include "utils.h"
#include "wave.h"

/* Sombra escura no topo para dar leitura ao texto sobre o fundo. */
static void DrawTopShade(void)
{
    const int bands = 8;
    for (int i = 0; i < bands; i++) {
        float t = 1.0f - (float)i / (float)bands;
        DrawRectangle(0, i * 9, SCREEN_WIDTH, 9, ColorWithAlpha(COL_BLACK, 0.55f * t));
    }
}

/* Número da onda + barra de progresso; na onda final, a barra de vida do chefe. */
static void DrawWaveInfo(const Game *g, float cx)
{
    if (g->wave.wave >= FINAL_WAVE) {
        Gfx_DrawTextCentered("ONDA FINAL", cx, 8.0f, 20.0f, COL_RED);

        const Enemy *boss = Enemy_FindBoss(g->enemies, MAX_ENEMIES);
        if (boss != NULL) {
            Rectangle bar = { cx - 260.0f, 36.0f, 520.0f, 16.0f };
            Gfx_DrawBar(bar, (float)boss->hp / (float)boss->maxHp, COL_RED, ColorWithAlpha(COL_BLACK, 0.55f));
            Gfx_DrawTextCentered("CHEFE", cx, 56.0f, 20.0f, ColorWithAlpha(COL_WHITE, 0.8f));
        }
    } else {
        Gfx_DrawTextCentered(TextFormat("ONDA %d", g->wave.wave), cx, 8.0f, 20.0f, COL_CYAN);
        Rectangle bar = { cx - 170.0f, 36.0f, 340.0f, 12.0f };
        Gfx_DrawBar(bar, Wave_Progress(&g->wave, g->score), COL_CYAN, ColorWithAlpha(COL_BLACK, 0.55f));
    }
}

/* Vidas restantes: ícones da nave no canto inferior esquerdo. */
static void DrawLives(const Game *g)
{
    Gfx_DrawText("VIDAS", 24.0f, SCREEN_HEIGHT - 74.0f, 20.0f, COL_GRAY);
    for (int i = 0; i < g->player.lives; i++) {
        Player_DrawShip((Vector2){ 40.0f + (float)i * 42.0f, SCREEN_HEIGHT - 32.0f }, 0.55f, 1.0f, false, 0.0f);
    }
}

/* Nível da arma e escudo ativo, no canto inferior direito. */
static void DrawPowerStatus(const Game *g)
{
    const float right = SCREEN_WIDTH - 24.0f;
    const float y = SCREEN_HEIGHT - 74.0f;

    Gfx_DrawTextRight("ARMA", right, y, 20.0f, COL_GRAY);
    for (int i = 0; i < PLAYER_MAX_WEAPON; i++) {
        Rectangle pip = { right - (float)(PLAYER_MAX_WEAPON - i) * 28.0f + 6.0f, y + 28.0f, 22.0f, 12.0f };
        if (i < g->player.weaponLevel) DrawRectangleRec(pip, COL_ORANGE);
        else DrawRectangleLinesEx(pip, 2.0f, ColorWithAlpha(COL_GRAY, 0.6f));
    }

    if (g->player.shieldTimer > 0.0f) {
        Rectangle bar = { right - 120.0f, y - 22.0f, 120.0f, 10.0f };
        Gfx_DrawTextRight("ESCUDO", right - 130.0f, y - 28.0f, 20.0f, COL_CYAN);
        Gfx_DrawBar(bar, g->player.shieldTimer / PLAYER_SHIELD_TIME, COL_CYAN, ColorWithAlpha(COL_BLACK, 0.55f));
    }
}

/* Borda vermelha pulsante quando resta apenas uma vida. */
static void DrawDangerBorder(const Game *g)
{
    if (g->player.alive && g->player.lives == 1) {
        float pulse = 0.5f + 0.5f * sinf(g->time * 6.0f);
        DrawRectangleLinesEx((Rectangle){ 0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT }, 8.0f,
                             ColorWithAlpha(COL_RED, 0.15f + 0.25f * pulse));
    }
}

/* Aviso "ONDA N" no início de cada onda. */
static void DrawWaveBanner(const Game *g)
{
    float t = g->wave.bannerTimer;
    if (t <= 0.0f) return;

    float elapsed = WAVE_BANNER_TIME - t;
    float alpha = Clamp(fminf(elapsed / 0.35f, t / 0.6f), 0.0f, 1.0f);   /* surge e some suavemente */
    float cx = SCREEN_WIDTH * 0.5f;
    float y = SCREEN_HEIGHT * 0.30f;

    if (g->wave.wave >= FINAL_WAVE) {
        Gfx_DrawTextNeon("ONDA FINAL", cx, y, 80.0f, ColorWithAlpha(COL_RED, alpha));
        Gfx_DrawTextCentered("O CHEFE ESTÁ CHEGANDO!", cx, y + 100.0f, 30.0f, ColorWithAlpha(COL_GOLD, alpha));
    } else {
        Gfx_DrawTextNeon(TextFormat("ONDA %d", g->wave.wave), cx, y, 80.0f, ColorWithAlpha(COL_WHITE, alpha));
        Gfx_DrawTextCentered("PREPARE-SE!", cx, y + 100.0f, 30.0f, ColorWithAlpha(COL_CYAN, alpha));
    }
}

void HUD_Draw(const Game *g)
{
    const float W = (float)SCREEN_WIDTH;
    const float cx = W * 0.5f;

    DrawTopShade();

    /* pontuação (esquerda) */
    Gfx_DrawText("PONTOS", 24.0f, 8.0f, 20.0f, COL_GRAY);
    Gfx_DrawText(TextFormat("%07d", g->score), 24.0f, 30.0f, 30.0f, COL_WHITE);

    /* recorde (direita): atualiza ao vivo quando a pontuação o ultrapassa */
    int record = (g->score > g->highScore) ? g->score : g->highScore;
    Gfx_DrawTextRight("RECORDE", W - 24.0f, 8.0f, 20.0f, COL_GRAY);
    Gfx_DrawTextRight(TextFormat("%07d", record), W - 24.0f, 30.0f, 30.0f, COL_GOLD);

    DrawWaveInfo(g, cx);
    DrawLives(g);
    DrawPowerStatus(g);
    DrawDangerBorder(g);
    DrawWaveBanner(g);

    /* indicador de dano: flash vermelho que some rapidamente */
    if (g->damageFlash > 0.0f) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorWithAlpha(COL_RED, g->damageFlash * 0.35f));
    }
}
