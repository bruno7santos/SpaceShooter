/*
 * gfx.h — Funções de desenho auxiliares (texto, brilho, polígonos, barras)
 */
#ifndef GFX_H
#define GFX_H

#include "raylib.h"

/* ---------- Paleta de cores do jogo ---------- */
#define COL_BLACK      ((Color){   0,   0,   0, 255 })
#define COL_WHITE      ((Color){ 255, 255, 255, 255 })
#define COL_WHITE_SOFT ((Color){ 225, 238, 255, 255 })
#define COL_CYAN       ((Color){   0, 229, 255, 255 })
#define COL_BLUE       ((Color){  70, 140, 255, 255 })
#define COL_MAGENTA    ((Color){ 255,  64, 200, 255 })
#define COL_PURPLE     ((Color){ 170,  90, 255, 255 })
#define COL_GOLD       ((Color){ 255, 214,  64, 255 })
#define COL_ORANGE     ((Color){ 255, 140,  40, 255 })
#define COL_RED        ((Color){ 255,  70,  80, 255 })
#define COL_GREEN      ((Color){  80, 230, 130, 255 })
#define COL_GRAY       ((Color){ 150, 160, 185, 255 })

/* Carrega/descarrega a fonte. Se existir assets/fonts/game_font.ttf ela é usada;
 * caso contrário o jogo usa a fonte padrão embutida na raylib. */
void  Gfx_Init(void);
void  Gfx_Unload(void);

/* ---------- Texto ---------- */
float Gfx_TextWidth(const char *text, float size);
void  Gfx_DrawText(const char *text, float x, float y, float size, Color color);
void  Gfx_DrawTextCentered(const char *text, float centerX, float y, float size, Color color);
void  Gfx_DrawTextRight(const char *text, float rightX, float y, float size, Color color);
void  Gfx_DrawTextNeon(const char *text, float centerX, float y, float size, Color color);

/* ---------- Formas ---------- */
/* Brilho suave. Use dentro de BeginBlendMode(BLEND_ADDITIVE) para o melhor efeito. */
void  Gfx_DrawGlow(Vector2 center, float radius, Color color);
/* Polígono convexo preenchido. Os pontos são relativos a "origin" e multiplicados por "scale". */
void  Gfx_FillPolygon(const Vector2 *points, int count, Vector2 origin, float scale, Color color);
void  Gfx_OutlinePolygon(const Vector2 *points, int count, Vector2 origin, float scale, float thickness, Color color);
void  Gfx_FillEllipse(Vector2 center, float radiusX, float radiusY, Color color);
/* Polígono regular (hexágono, octógono...). Com rotação 0, um vértice aponta para a direita. */
void  Gfx_FillRegular(Vector2 center, int sides, float radius, float rotationDeg, Color color);
void  Gfx_OutlineRegular(Vector2 center, int sides, float radius, float rotationDeg, float thickness, Color color);

/* ---------- Interface ---------- */
void  Gfx_DrawBar(Rectangle bounds, float fraction, Color fill, Color background);
void  Gfx_DrawPanel(Rectangle bounds, Color fill, Color border);

#endif /* GFX_H */
