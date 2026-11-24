/*
 * game.c — Estados do jogo, regras principais e laço de atualização/desenho
 */
#include "game.h"

#include <math.h>
#include <string.h>
#include <time.h>

#include "audio.h"
#include "collision.h"
#include "gfx.h"
#include "highscore.h"
#include "hud.h"
#include "screens.h"
#include "utils.h"

/* ================================================================== */
/*  Funções auxiliares internas                                        */
/* ================================================================== */

static void ChangeState(Game *g, GameState state)
{
    g->state = state;
    if (state == STATE_PLAYING) HideCursor();   /* o mouse só é usado nos menus */
    else ShowCursor();
}

/* Grava o recorde se a pontuação atual o superou. */
static void CommitScore(Game *g)
{
    if (g->score > g->highScore) {
        g->highScore = g->score;
        HighScore_Save(g->highScore);
    }
}

/* Zera tudo e começa uma partida nova. */
static void StartRun(Game *g)
{
    Player_Init(&g->player);
    Enemy_ClearAll(g->enemies, MAX_ENEMIES);
    Projectile_ClearAll(g->projectiles, MAX_PROJECTILES);
    PowerUp_ClearAll(g->powerUps, MAX_POWERUPS);
    Particles_Clear(&g->particles);
    Wave_Init(&g->wave);

    g->score = 0;
    g->newRecord = false;
    g->victoryBonus = 0;
    g->runTime = 0.0f;
    g->shake = 0.0f;
    g->shakeOffset = (Vector2){ 0.0f, 0.0f };
    g->damageFlash = 0.0f;
    g->gameOverTimer = 0.0f;
    g->victoryTimer = 0.0f;
    g->blastTimer = 0.0f;

    Audio_MusicDuck(false);
    Audio_MusicStart();
    Audio_Play(SFX_WAVE);
    ChangeState(g, STATE_PLAYING);
}

/* Encerra a partida (derrota ou vitória) e mostra a tela final. */
static void FinishRun(Game *g, bool victory)
{
    if (victory) {
        g->victoryBonus = g->player.lives * VICTORY_LIFE_BONUS;
        g->score += g->victoryBonus;
    }

    g->newRecord = (g->score > g->highScore);
    CommitScore(g);

    Projectile_ClearAll(g->projectiles, MAX_PROJECTILES);   /* nada fica "congelado" atrás da tela final */
    Audio_MusicStop();
    Audio_Play(victory ? SFX_VICTORY : SFX_GAME_OVER);
    Menu_Reset(&g->endMenu, 0.9f);
    ChangeState(g, victory ? STATE_VICTORY : STATE_GAME_OVER);
}

static void ReturnToMenu(Game *g)
{
    CommitScore(g);
    Menu_Reset(&g->mainMenu, 0.15f);
    Audio_MusicDuck(false);
    Audio_MusicStart();
    ChangeState(g, STATE_MENU);
}

/* ================================================================== */
/*  Regras (chamadas pelo módulo de colisão)                           */
/* ================================================================== */

/* Chance de um inimigo destruído soltar um power-up. */
static float DropChance(EnemyType type)
{
    switch (type) {
        case ENEMY_TANK:    return 0.40f;
        case ENEMY_SHOOTER: return 0.25f;
        default:            return 0.09f;
    }
}

static PowerUpType RandomPowerUpType(const Player *p)
{
    int roll = GetRandomValue(0, 99);
    if (roll < 50) return POWERUP_WEAPON;
    if (roll < 85) return POWERUP_SHIELD;
    return (p->lives < PLAYER_MAX_LIVES) ? POWERUP_LIFE : POWERUP_SHIELD;
}

/* Chefe derrotado: explosões em série e depois a tela de vitória. */
static void BeginVictorySequence(Game *g, Vector2 bossPos)
{
    g->victoryTimer = VICTORY_DELAY;
    g->blastTimer = 0.0f;
    g->bossPos = bossPos;
    g->shake = 24.0f;
    Audio_Play(SFX_BIG_EXPLOSION);

    /* remove os tiros inimigos e destrói os reforços que sobraram */
    Projectile_ClearOwner(g->projectiles, MAX_PROJECTILES, OWNER_ENEMY);
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &g->enemies[i];
        if (!e->active) continue;
        e->active = false;
        Particles_Explosion(&g->particles, e->position, Enemy_Color(e->type), 24, 260.0f);
    }
}

/* O chefe entrou em uma fase mais agressiva: avisa o jogador e dá uma ajuda (power-up). */
static void OnBossPhaseChange(Game *g, const Enemy *boss)
{
    PowerUpType help = POWERUP_SHIELD;
    if (Enemy_BossPhase(boss) >= 3 && g->player.lives < PLAYER_MAX_LIVES) help = POWERUP_LIFE;

    Particles_AddPopup(&g->particles, (Vector2){ boss->position.x, boss->position.y + boss->radius + 10.0f },
                       "CHEFE ENFURECIDO!", COL_RED);
    Particles_Explosion(&g->particles, boss->position, COL_MAGENTA, 30, 300.0f);
    PowerUp_Spawn(g->powerUps, MAX_POWERUPS, (Vector2){ boss->position.x, boss->position.y + boss->radius }, help);
    Audio_Play(SFX_WAVE);
    g->shake = fmaxf(g->shake, 10.0f);
}

void Game_OnEnemyHit(Game *g, Enemy *e, int damage, Vector2 hitPosition)
{
    /* o chefe entrando na tela é invulnerável: o tiro só gera faíscas */
    if (!Enemy_IsVulnerable(e)) {
        Particles_Sparks(&g->particles, hitPosition, COL_WHITE_SOFT, 4);
        return;
    }

    int phaseBefore = (e->type == ENEMY_BOSS) ? Enemy_BossPhase(e) : 0;

    if (!Enemy_Hit(e, damage)) {                         /* levou dano, mas sobreviveu */
        Particles_Sparks(&g->particles, hitPosition, COL_CYAN, 5);
        Audio_Play(SFX_HIT);
        if (e->type == ENEMY_BOSS && Enemy_BossPhase(e) > phaseBefore) OnBossPhaseChange(g, e);
        return;
    }

    /* ---- inimigo destruído ---- */
    g->score += e->points;

    bool big = (e->type == ENEMY_TANK || e->type == ENEMY_BOSS);
    Particles_Explosion(&g->particles, e->position, Enemy_Color(e->type),
                        (int)(18.0f + e->radius * 0.9f), 220.0f + e->radius * 3.0f);
    Particles_AddPopup(&g->particles, e->position, TextFormat("+%d", e->points), Enemy_Color(e->type));
    Audio_Play(big ? SFX_BIG_EXPLOSION : SFX_EXPLOSION);
    g->shake = fmaxf(g->shake, big ? 8.0f : 3.0f);

    if (e->type == ENEMY_BOSS) {
        /* se o jogador já tinha morrido, a derrota prevalece */
        if (g->player.alive) BeginVictorySequence(g, e->position);
        return;
    }

    if (RandFloat(0.0f, 1.0f) < DropChance(e->type)) {
        PowerUp_Spawn(g->powerUps, MAX_POWERUPS, e->position, RandomPowerUpType(&g->player));
    }
}

/* O jogador bateu em um inimigo comum: ele explode (sem dar pontos). */
void Game_OnEnemyRammed(Game *g, Enemy *e)
{
    e->active = false;
    Particles_Explosion(&g->particles, e->position, Enemy_Color(e->type), 20, 240.0f);
    Audio_Play(SFX_EXPLOSION);
}

void Game_OnPlayerHit(Game *g)
{
    if (!g->player.alive || g->victoryTimer > 0.0f) return;

    HitResult result = Player_TakeDamage(&g->player);
    if (result == HIT_IGNORED) return;

    Vector2 pos = g->player.position;

    if (result == HIT_SHIELD_BLOCKED) {                  /* escudo absorveu o golpe */
        Particles_Sparks(&g->particles, pos, COL_CYAN, 18);
        Audio_Play(SFX_SHIELD);
        g->shake = fmaxf(g->shake, 4.0f);
        return;
    }

    /* perdeu uma vida */
    g->damageFlash = 1.0f;
    g->shake = fmaxf(g->shake, 12.0f);
    Particles_Explosion(&g->particles, pos, COL_CYAN, 26, 260.0f);

    if (g->player.lives > 0) {
        Audio_Play(SFX_PLAYER_HIT);
        return;
    }

    /* acabaram as vidas: explosão final e uma pequena espera antes do Game Over */
    g->player.alive = false;
    g->gameOverTimer = GAME_OVER_DELAY;
    Particles_Explosion(&g->particles, pos, COL_WHITE_SOFT, 60, 420.0f);
    Audio_Play(SFX_BIG_EXPLOSION);
    g->shake = 20.0f;
}

void Game_OnPowerUpCollected(Game *g, PowerUp *pu)
{
    const char *text = "";

    pu->active = false;
    Audio_Play(SFX_POWERUP);
    Particles_Sparks(&g->particles, pu->position, PowerUp_Color(pu->type), 16);

    switch (pu->type) {
        case POWERUP_WEAPON:
            if (Player_UpgradeWeapon(&g->player)) {
                text = TextFormat("ARMA NÍVEL %d", g->player.weaponLevel);
            } else {                                    /* já está no máximo: vira bônus */
                g->score += 500;
                text = "+500";
            }
            break;

        case POWERUP_SHIELD:
            Player_GiveShield(&g->player);
            text = "ESCUDO!";
            break;

        case POWERUP_LIFE:
            if (Player_AddLife(&g->player)) {
                text = "+1 VIDA";
            } else {
                g->score += 500;
                text = "+500";
            }
            break;

        default:
            break;
    }
    Particles_AddPopup(&g->particles, pu->position, text, PowerUp_Color(pu->type));
}

/* ================================================================== */
/*  Atualização por estado                                             */
/* ================================================================== */

static void UpdateMenu(Game *g, float dt)
{
    Background_Update(&g->background, dt, 0.8f);

    int choice = Menu_Update(&g->mainMenu, dt);
    if (choice == 0) StartRun(g);
    else if (choice == 1) g->shouldQuit = true;
}

static void HandleWaveEvent(Game *g, WaveEvent event)
{
    switch (event) {
        case WAVE_EVENT_NEW_WAVE:
            Audio_Play(SFX_WAVE);
            break;
        case WAVE_EVENT_FINAL_WAVE:
            Audio_Play(SFX_WAVE);
            g->shake = fmaxf(g->shake, 6.0f);
            break;
        case WAVE_EVENT_BOSS_SPAWNED:
            /* presente: um power-up de arma cai junto com a chegada do chefe */
            if (g->player.weaponLevel < PLAYER_MAX_WEAPON) {
                PowerUp_Spawn(g->powerUps, MAX_POWERUPS, (Vector2){ SCREEN_WIDTH * 0.5f, -20.0f }, POWERUP_WEAPON);
            }
            break;
        default:
            break;
    }
}

static void UpdateEffects(Game *g, float dt)
{
    g->shake = fmaxf(0.0f, g->shake - 40.0f * dt);
    g->shakeOffset = (Vector2){ RandFloat(-g->shake, g->shake), RandFloat(-g->shake, g->shake) };
    g->damageFlash = fmaxf(0.0f, g->damageFlash - 2.2f * dt);
}

static void UpdatePlaying(Game *g, float dt)
{
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {       /* pausa */
        Menu_Reset(&g->pauseMenu, 0.0f);
        Audio_MusicDuck(true);
        ChangeState(g, STATE_PAUSED);
        return;
    }

    g->runTime += dt;
    Background_Update(&g->background, dt, 1.0f + 0.12f * (float)(g->wave.wave - 1));

    /* jogador */
    if (g->player.alive) {
        Player_Update(&g->player, dt, g->projectiles, MAX_PROJECTILES);
        Particles_Thruster(&g->particles, g->player.position, dt);
    }

    /* ondas e criação de inimigos (parada durante as sequências de fim) */
    bool spawnAllowed = g->player.alive && g->victoryTimer <= 0.0f;
    HandleWaveEvent(g, Wave_Update(&g->wave, dt, g->score, g->enemies, MAX_ENEMIES, spawnAllowed));

    /* demais entidades */
    EnemyContext ctx = { g->player.position, g->projectiles, MAX_PROJECTILES };
    Enemy_UpdateAll(g->enemies, MAX_ENEMIES, dt, &ctx);
    Projectile_UpdateAll(g->projectiles, MAX_PROJECTILES, dt);
    PowerUp_UpdateAll(g->powerUps, MAX_POWERUPS, dt);

    Collision_Update(g);

    Particles_Update(&g->particles, dt);
    UpdateEffects(g, dt);

    /* nave destruída: espera a explosão terminar e mostra o Game Over */
    if (g->gameOverTimer > 0.0f) {
        g->gameOverTimer -= dt;
        if (g->gameOverTimer <= 0.0f) {
            FinishRun(g, false);
            return;
        }
    }

    /* chefe derrotado: explosões em série e depois a Vitória */
    if (g->victoryTimer > 0.0f) {
        g->blastTimer -= dt;
        if (g->blastTimer <= 0.0f) {
            Vector2 pos = { g->bossPos.x + RandFloat(-90.0f, 90.0f), g->bossPos.y + RandFloat(-70.0f, 70.0f) };
            g->blastTimer = 0.16f;
            Particles_Explosion(&g->particles, pos, (GetRandomValue(0, 1) == 0) ? COL_GOLD : COL_ORANGE, 26, 300.0f);
            Audio_Play(SFX_EXPLOSION);
            g->shake = fmaxf(g->shake, 8.0f);
        }
        g->victoryTimer -= dt;
        if (g->victoryTimer <= 0.0f) FinishRun(g, true);
    }
}

static void UpdatePaused(Game *g, float dt)
{
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
        Audio_MusicDuck(false);
        ChangeState(g, STATE_PLAYING);
        return;
    }

    int choice = Menu_Update(&g->pauseMenu, dt);
    if (choice == 0) {                                  /* continuar */
        Audio_MusicDuck(false);
        ChangeState(g, STATE_PLAYING);
    } else if (choice == 1) {                           /* voltar ao menu */
        ReturnToMenu(g);
    }
}

static void UpdateEnd(Game *g, float dt)
{
    Background_Update(&g->background, dt, 0.6f);
    Particles_Update(&g->particles, dt);
    UpdateEffects(g, dt);

    int choice = Menu_Update(&g->endMenu, dt);
    if (choice == 0) StartRun(g);                       /* jogar novamente */
    else if (choice == 1) ReturnToMenu(g);              /* menu principal */
}

/* ================================================================== */
/*  API pública                                                        */
/* ================================================================== */

void Game_Init(Game *g)
{
    static const char *const mainItems[]  = { "INICIAR JOGO", "SAIR" };
    static const char *const pauseItems[] = { "CONTINUAR", "VOLTAR AO MENU" };
    static const char *const endItems[]   = { "JOGAR NOVAMENTE", "MENU PRINCIPAL" };
    const float cx = SCREEN_WIDTH * 0.5f;

    memset(g, 0, sizeof *g);
    SetRandomSeed((unsigned int)time(NULL));

    g->highScore = HighScore_Load();
    Background_Init(&g->background);
    Particles_Init(&g->particles);
    Wave_Init(&g->wave);

    Menu_Init(&g->mainMenu,  mainItems,  2, cx, 300.0f, 70.0f);
    Menu_Init(&g->pauseMenu, pauseItems, 2, cx, 330.0f, 70.0f);
    Menu_Init(&g->endMenu,   endItems,   2, cx, 500.0f, 70.0f);

    ChangeState(g, STATE_MENU);
    Audio_MusicStart();
}

void Game_Update(Game *g, float dt)
{
    g->time += dt;

    if (IsKeyPressed(KEY_M)) Audio_MusicToggle();
    if (IsKeyPressed(KEY_F3)) g->showFps = !g->showFps;

    switch (g->state) {
        case STATE_MENU:      UpdateMenu(g, dt);    break;
        case STATE_PLAYING:   UpdatePlaying(g, dt); break;
        case STATE_PAUSED:    UpdatePaused(g, dt);  break;
        case STATE_GAME_OVER:
        case STATE_VICTORY:   UpdateEnd(g, dt);     break;
    }
}

/* Desenha o "mundo" do jogo (sujeito ao tremor de tela). O fundo e o HUD não tremem. */
static void DrawWorld(const Game *g)
{
    Camera2D camera = { .offset = g->shakeOffset, .target = { 0.0f, 0.0f }, .rotation = 0.0f, .zoom = 1.0f };

    BeginMode2D(camera);
        PowerUp_DrawAll(g->powerUps, MAX_POWERUPS);
        Enemy_DrawAll(g->enemies, MAX_ENEMIES);
        Projectile_DrawAll(g->projectiles, MAX_PROJECTILES);
        Player_Draw(&g->player);
        Particles_Draw(&g->particles);
    EndMode2D();
}

void Game_Draw(const Game *g)
{
    ClearBackground(COL_BLACK);
    Background_Draw(&g->background);

    switch (g->state) {
        case STATE_MENU:
            Screen_DrawMenu(g);
            break;
        case STATE_PLAYING:
            DrawWorld(g);
            HUD_Draw(g);
            break;
        case STATE_PAUSED:
            DrawWorld(g);
            HUD_Draw(g);
            Screen_DrawPause(g);
            break;
        case STATE_GAME_OVER:
            DrawWorld(g);
            Screen_DrawGameOver(g);
            break;
        case STATE_VICTORY:
            DrawWorld(g);
            Screen_DrawVictory(g);
            break;
    }

    if (g->showFps) {
        Gfx_DrawText(TextFormat("%d FPS", GetFPS()), 10.0f, SCREEN_HEIGHT - 28.0f, 20.0f, COL_GREEN);
    }
}

void Game_Unload(Game *g)
{
    CommitScore(g);      /* não perde o recorde se a janela for fechada no meio da partida */
    ShowCursor();
}
