/*
 * gfx.c — Funções de desenho auxiliares
 */
#include "gfx.h"

#include <math.h>
#include "utils.h"

#define FONT_PATH       "assets/fonts/game_font.ttf"
#define GFX_MAX_POINTS  32

/* Estado privado do módulo: a fonte em uso e o espaçamento entre letras. */
static Font  gFont;
static bool  gHasCustomFont = false;
static float gSpacing = 0.1f;   /* espaçamento proporcional ao tamanho do texto */

void Gfx_Init(void)
{
    gFont = GetFontDefault();
    gHasCustomFont = false;
    gSpacing = 0.1f;

    if (FileExists(FONT_PATH)) {
        /* Caracteres ASCII (32..126) + Latin-1 (160..255): cobre os acentos do português. */
        int codepoints[224];
        int count = 0;
        for (int c = 32; c <= 126; c++) codepoints[count++] = c;
        for (int c = 160; c <= 255; c++) codepoints[count++] = c;

        Font custom = LoadFontEx(FONT_PATH, 64, codepoints, count);
        if (custom.texture.id != 0) {
            SetTextureFilter(custom.texture, TEXTURE_FILTER_BILINEAR);
            gFont = custom;
            gHasCustomFont = true;
            gSpacing = 0.04f;
        }
    }
}

void Gfx_Unload(void)
{
    if (gHasCustomFont) {
        UnloadFont(gFont);
        gHasCustomFont = false;
    }
}

/* ------------------------------------------------------------------ */
/*  Texto                                                              */
/* ------------------------------------------------------------------ */

float Gfx_TextWidth(const char *text, float size)
{
    return MeasureTextEx(gFont, text, size, size * gSpacing).x;
}

void Gfx_DrawText(const char *text, float x, float y, float size, Color color)
{
    /* Posições inteiras deixam a fonte pixelada mais nítida. */
    Vector2 pos = { floorf(x), floorf(y) };
    DrawTextEx(gFont, text, pos, size, size * gSpacing, color);
}

void Gfx_DrawTextCentered(const char *text, float centerX, float y, float size, Color color)
{
    Gfx_DrawText(text, centerX - Gfx_TextWidth(text, size) * 0.5f, y, size, color);
}

void Gfx_DrawTextRight(const char *text, float rightX, float y, float size, Color color)
{
    Gfx_DrawText(text, rightX - Gfx_TextWidth(text, size), y, size, color);
}

/* Efeito "neon": duas cópias coloridas deslocadas atrás do texto principal. */
void Gfx_DrawTextNeon(const char *text, float centerX, float y, float size, Color color)
{
    float a = (float)color.a / 255.0f;
    Gfx_DrawTextCentered(text, centerX - 3.0f, y + 2.0f, size, ColorWithAlpha(COL_MAGENTA, 0.75f * a));
    Gfx_DrawTextCentered(text, centerX + 3.0f, y - 2.0f, size, ColorWithAlpha(COL_CYAN, 0.75f * a));
    Gfx_DrawTextCentered(text, centerX, y, size, color);
}

/* ------------------------------------------------------------------ */
/*  Formas                                                             */
/* ------------------------------------------------------------------ */

void Gfx_DrawGlow(Vector2 center, float radius, Color color)
{
    /* Círculos concêntricos pouco opacos: com blend aditivo viram um brilho suave. */
    DrawCircleV(center, radius,         ColorWithAlpha(color, 0.07f));
    DrawCircleV(center, radius * 0.72f, ColorWithAlpha(color, 0.10f));
    DrawCircleV(center, radius * 0.48f, ColorWithAlpha(color, 0.16f));
    DrawCircleV(center, radius * 0.26f, ColorWithAlpha(color, 0.30f));
}

/*
 * Desenha um polígono CONVEXO dividindo-o em triângulos ("leque").
 * Atenção: DrawTriangle da raylib só desenha se os vértices estiverem em sentido
 * anti-horário. Aqui calculamos a área com sinal e invertemos a ordem quando preciso,
 * assim as formas dos sprites podem ser escritas em qualquer sentido.
 */
void Gfx_FillPolygon(const Vector2 *points, int count, Vector2 origin, float scale, Color color)
{
    Vector2 p[GFX_MAX_POINTS];
    float area = 0.0f;

    if (count < 3 || count > GFX_MAX_POINTS) return;

    for (int i = 0; i < count; i++) {
        p[i].x = origin.x + points[i].x * scale;
        p[i].y = origin.y + points[i].y * scale;
    }
    for (int i = 0; i < count; i++) {
        int j = (i + 1) % count;
        area += p[i].x * p[j].y - p[j].x * p[i].y;
    }

    for (int i = 1; i < count - 1; i++) {
        if (area < 0.0f) DrawTriangle(p[0], p[i], p[i + 1], color);
        else             DrawTriangle(p[0], p[i + 1], p[i], color);
    }
}

void Gfx_OutlinePolygon(const Vector2 *points, int count, Vector2 origin, float scale, float thickness, Color color)
{
    for (int i = 0; i < count; i++) {
        int j = (i + 1) % count;
        Vector2 a = { origin.x + points[i].x * scale, origin.y + points[i].y * scale };
        Vector2 b = { origin.x + points[j].x * scale, origin.y + points[j].y * scale };
        DrawLineEx(a, b, thickness, color);
    }
}

void Gfx_FillEllipse(Vector2 center, float radiusX, float radiusY, Color color)
{
    enum { SEGMENTS = 20 };
    Vector2 pts[SEGMENTS];

    for (int i = 0; i < SEGMENTS; i++) {
        float angle = (2.0f * PI * (float)i) / (float)SEGMENTS;
        pts[i].x = cosf(angle) * radiusX;
        pts[i].y = sinf(angle) * radiusY;
    }
    Gfx_FillPolygon(pts, SEGMENTS, center, 1.0f, color);
}

/* Calcula os vértices de um polígono regular (centro na origem). */
static int RegularPoints(Vector2 *out, int sides, float radius, float rotationDeg)
{
    if (sides < 3) sides = 3;
    if (sides > GFX_MAX_POINTS) sides = GFX_MAX_POINTS;

    for (int i = 0; i < sides; i++) {
        float angle = rotationDeg * DEG2RAD + (2.0f * PI * (float)i) / (float)sides;
        out[i].x = cosf(angle) * radius;
        out[i].y = sinf(angle) * radius;
    }
    return sides;
}

void Gfx_FillRegular(Vector2 center, int sides, float radius, float rotationDeg, Color color)
{
    Vector2 pts[GFX_MAX_POINTS];
    int n = RegularPoints(pts, sides, radius, rotationDeg);
    Gfx_FillPolygon(pts, n, center, 1.0f, color);
}

void Gfx_OutlineRegular(Vector2 center, int sides, float radius, float rotationDeg, float thickness, Color color)
{
    Vector2 pts[GFX_MAX_POINTS];
    int n = RegularPoints(pts, sides, radius, rotationDeg);
    Gfx_OutlinePolygon(pts, n, center, 1.0f, thickness, color);
}

/* ------------------------------------------------------------------ */
/*  Interface                                                          */
/* ------------------------------------------------------------------ */

void Gfx_DrawBar(Rectangle bounds, float fraction, Color fill, Color background)
{
    if (fraction < 0.0f) fraction = 0.0f;
    if (fraction > 1.0f) fraction = 1.0f;

    DrawRectangleRec(bounds, background);
    if (fraction > 0.0f) {
        Rectangle filled = { bounds.x, bounds.y, bounds.width * fraction, bounds.height };
        DrawRectangleRec(filled, fill);
    }
    DrawRectangleLinesEx(bounds, 1.0f, ColorWithAlpha(COL_WHITE, 0.35f));
}

void Gfx_DrawPanel(Rectangle bounds, Color fill, Color border)
{
    DrawRectangleRec(bounds, fill);
    DrawRectangleLinesEx(bounds, 2.0f, border);
}
