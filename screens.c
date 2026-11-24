/*
 * screens.c — Telas do jogo (menu, pausa, game over, vitória)
 * Os textos usam tamanhos múltiplos de 10 porque a fonte padrão da raylib é
 * desenhada em 10 px: assim os pixels ficam nítidos.
 */
#include "screens.h"

#include <math.h>
#include "audio.h"
#include "config.h"
#include "gfx.h"
#include "menu.h"
#include "utils.h"

/* Uma linha "RÓTULO ........ valor" do painel de controles. */
static void DrawControlRow(float x, float y, const char *label, const char *value)
{
    Gfx_DrawText(label, x, y, 20.0f, COL_GOLD);
    Gfx_DrawText(value, x + 190.0f, y, 20.0f, COL_WHITE_SOFT);
}

/* Linha centralizada "rótulo: valor" (usada nas telas de fim de jogo). */
static void DrawStat(float cx, float y, const char *label, const char *value, Color valueColor, float valueSize)
{
    Gfx_DrawTextCentered(label, cx, y, 20.0f, COL_GRAY);
    Gfx_DrawTextCentered(value, cx, y + 24.0f, valueSize, valueColor);
}

void Screen_DrawMenu(const Game *g)
{
    const float cx = SCREEN_WIDTH * 0.5f;

    /* "frota" decorativa nos lados do menu */
    Player_DrawShip((Vector2){ cx + 400.0f, 350.0f + sinf(g->time * 1.6f) * 14.0f }, 2.4f, 1.0f, true, g->time);
    Enemy_DrawPreview(ENEMY_ZIGZAG,  (Vector2){ cx - 400.0f, 300.0f + sinf(g->time * 1.3f) * 12.0f }, 1.9f, g->time);
    Enemy_DrawPreview(ENEMY_SHOOTER, (Vector2){ cx - 330.0f, 420.0f + sinf(g->time * 1.1f + 1.0f) * 10.0f }, 1.5f, g->time);
    Enemy_DrawPreview(ENEMY_KAMIKAZE,(Vector2){ cx - 470.0f, 430.0f + sinf(g->time * 1.7f + 2.0f) * 10.0f }, 1.5f, g->time);

    Gfx_DrawTextNeon("SPACE SHOOTER", cx, 80.0f, 100.0f, COL_WHITE);
    Gfx_DrawTextCentered("Sobreviva às ondas e derrote o chefe!", cx, 205.0f, 30.0f, COL_GRAY);

    Menu_Draw(&g->mainMenu, g->time);

    Gfx_DrawTextCentered(TextFormat("RECORDE: %07d", g->highScore), cx, 440.0f, 20.0f, COL_GOLD);

    /* painel de controles */
    Rectangle panel = { cx - 330.0f, 490.0f, 660.0f, 190.0f };
    Gfx_DrawPanel(panel, ColorWithAlpha(COL_BLACK, 0.45f), ColorWithAlpha(COL_CYAN, 0.5f));
    Gfx_DrawTextCentered("CONTROLES", cx, panel.y + 12.0f, 20.0f, COL_CYAN);
    DrawControlRow(panel.x + 60.0f, panel.y + 48.0f,  "MOVER",   "W A S D  ou  SETAS");
    DrawControlRow(panel.x + 60.0f, panel.y + 76.0f,  "ATIRAR",  "ESPAÇO (segure)");
    DrawControlRow(panel.x + 60.0f, panel.y + 104.0f, "PAUSAR",  "ESC  ou  P");
    DrawControlRow(panel.x + 60.0f, panel.y + 132.0f, "MÚSICA",  Audio_MusicEnabled() ? "M  (ligada)" : "M  (desligada)");
}

void Screen_DrawPause(const Game *g)
{
    const float cx = SCREEN_WIDTH * 0.5f;

    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorWithAlpha(COL_BLACK, 0.60f));

    Rectangle panel = { cx - 290.0f, 170.0f, 580.0f, 380.0f };
    Gfx_DrawPanel(panel, ColorWithAlpha((Color){ 8, 12, 34, 255 }, 0.90f), ColorWithAlpha(COL_CYAN, 0.7f));

    Gfx_DrawTextNeon("PAUSA", cx, 200.0f, 80.0f, COL_WHITE);
    Menu_Draw(&g->pauseMenu, g->time);
    Gfx_DrawTextCentered("ESC para continuar   |   M música", cx, 490.0f, 20.0f, COL_GRAY);
}

void Screen_DrawGameOver(const Game *g)
{
    const float cx = SCREEN_WIDTH * 0.5f;

    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorWithAlpha((Color){ 40, 0, 12, 255 }, 0.62f));

    Gfx_DrawTextNeon("GAME OVER", cx, 90.0f, 100.0f, COL_RED);

    DrawStat(cx, 235.0f, "PONTUAÇÃO FINAL", TextFormat("%07d", g->score), COL_WHITE, 60.0f);
    DrawStat(cx, 340.0f, "RECORDE", TextFormat("%07d", g->highScore), COL_GOLD, 40.0f);

    if (g->newRecord && BlinkOn(g->time, 0.8f)) {
        Gfx_DrawTextCentered("NOVO RECORDE!", cx, 430.0f, 30.0f, COL_GOLD);
    }

    Menu_Draw(&g->endMenu, g->time);
}

void Screen_DrawVictory(const Game *g)
{
    const float cx = SCREEN_WIDTH * 0.5f;

    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ColorWithAlpha((Color){ 6, 12, 40, 255 }, 0.62f));

    Gfx_DrawTextNeon("VITÓRIA!", cx, 70.0f, 100.0f, COL_GOLD);
    Gfx_DrawTextCentered("Você derrotou o chefe e salvou a galáxia!", cx, 190.0f, 30.0f, COL_WHITE_SOFT);

    DrawStat(cx, 250.0f, "PONTUAÇÃO FINAL", TextFormat("%07d", g->score), COL_WHITE, 60.0f);
    Gfx_DrawTextCentered(TextFormat("Bônus de vidas: +%d", g->victoryBonus), cx, 345.0f, 20.0f, COL_GREEN);
    DrawStat(cx, 380.0f, "RECORDE", TextFormat("%07d", g->highScore), COL_GOLD, 30.0f);

    if (g->newRecord && BlinkOn(g->time, 0.8f)) {
        Gfx_DrawTextCentered("NOVO RECORDE!", cx, 450.0f, 30.0f, COL_GOLD);
    }

    Menu_Draw(&g->endMenu, g->time);
}
