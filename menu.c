/*
 * menu.c — Componente de menu
 */
#include "menu.h"

#include <math.h>
#include "audio.h"
#include "gfx.h"
#include "utils.h"

#define ITEM_WIDTH   420.0f
#define ITEM_HEIGHT  56.0f
#define ITEM_FONT    30.0f

void Menu_Init(Menu *m, const char *const *labels, int count, float centerX, float startY, float spacing)
{
    if (count > MENU_MAX_ITEMS) count = MENU_MAX_ITEMS;

    for (int i = 0; i < count; i++) m->items[i] = labels[i];
    m->count = count;
    m->selected = 0;
    m->centerX = centerX;
    m->startY = startY;
    m->spacing = spacing;
    m->inputLock = 0.0f;
}

void Menu_Reset(Menu *m, float lockSeconds)
{
    m->selected = 0;
    m->inputLock = lockSeconds;
}

static Rectangle ItemRect(const Menu *m, int index)
{
    Rectangle r = { m->centerX - ITEM_WIDTH * 0.5f, m->startY + (float)index * m->spacing, ITEM_WIDTH, ITEM_HEIGHT };
    return r;
}

int Menu_Update(Menu *m, float dt)
{
    if (m->count <= 0) return -1;
    if (m->inputLock > 0.0f) m->inputLock -= dt;

    int previous = m->selected;
    bool clicked = false;

    /* teclado */
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) m->selected = (m->selected + 1) % m->count;
    if (IsKeyPressed(KEY_UP)   || IsKeyPressed(KEY_W)) m->selected = (m->selected + m->count - 1) % m->count;

    /* mouse: passar por cima seleciona, clicar confirma */
    Vector2 mouse = GetMousePosition();
    Vector2 delta = GetMouseDelta();
    bool mouseMoved = (delta.x != 0.0f || delta.y != 0.0f);
    for (int i = 0; i < m->count; i++) {
        if (!CheckCollisionPointRec(mouse, ItemRect(m, i))) continue;
        if (mouseMoved) m->selected = i;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            m->selected = i;
            clicked = true;
        }
    }

    if (m->selected != previous) Audio_Play(SFX_MENU_MOVE);

    bool confirm = clicked || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE);
    if (confirm && m->inputLock <= 0.0f) {
        Audio_Play(SFX_MENU_SELECT);
        return m->selected;
    }
    return -1;
}

void Menu_Draw(const Menu *m, float time)
{
    for (int i = 0; i < m->count; i++) {
        Rectangle r = ItemRect(m, i);
        float textY = r.y + (r.height - ITEM_FONT) * 0.5f;

        if (i == m->selected) {
            float pulse = 0.5f + 0.5f * sinf(time * 6.0f);
            Gfx_DrawPanel(r, ColorWithAlpha(COL_CYAN, 0.16f + 0.10f * pulse), COL_CYAN);
            Gfx_DrawTextCentered(m->items[i], m->centerX, textY, ITEM_FONT, COL_WHITE);
            Gfx_DrawText(">", r.x + 18.0f, textY, ITEM_FONT, COL_CYAN);
            Gfx_DrawText("<", r.x + r.width - 18.0f - Gfx_TextWidth("<", ITEM_FONT), textY, ITEM_FONT, COL_CYAN);
        } else {
            Gfx_DrawPanel(r, ColorWithAlpha(COL_BLACK, 0.35f), ColorWithAlpha(COL_GRAY, 0.35f));
            Gfx_DrawTextCentered(m->items[i], m->centerX, textY, ITEM_FONT, COL_GRAY);
        }
    }
}
