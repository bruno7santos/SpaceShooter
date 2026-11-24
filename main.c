/*
 * main.c — Ponto de entrada do Space Shooter
 * ------------------------------------------------------------------
 * O laço principal faz três coisas por frame: atualizar, tocar áudio e desenhar.
 * Toda a lógica fica nos módulos; aqui só inicializamos e liberamos recursos.
 */
#include "raylib.h"

#include "audio.h"
#include "config.h"
#include "game.h"
#include "gfx.h"

int main(void)
{
    /* VSYNC e antisserrilhado (MSAA) deixam as formas vetoriais mais suaves */
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, GAME_TITLE);
    SetExitKey(KEY_NULL);            /* por padrão ESC fecha a janela; aqui ele serve para pausar */
    SetTargetFPS(TARGET_FPS);

    Gfx_Init();
    Audio_Init();

    Game game;
    Game_Init(&game);

    while (!WindowShouldClose() && !game.shouldQuit) {
        /* delta time: toda movimentação usa dt, então o jogo roda igual em qualquer FPS */
        float dt = GetFrameTime();
        if (dt > MAX_FRAME_TIME) dt = MAX_FRAME_TIME;

        Game_Update(&game, dt);
        Audio_Update();

        BeginDrawing();
        Game_Draw(&game);
        EndDrawing();
    }

    /* libera tudo o que foi carregado */
    Game_Unload(&game);
    Audio_Unload();
    Gfx_Unload();
    CloseWindow();
    return 0;
}
