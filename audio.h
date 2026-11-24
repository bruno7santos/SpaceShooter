/*
 * audio.h — Efeitos sonoros e música
 * ------------------------------------------------------------------
 * Nenhum arquivo de áudio é necessário: todos os sons são SINTETIZADOS
 * por código quando o jogo inicia (ondas senoidais, quadradas, ruído...).
 * Se existir assets/sounds/music.ogg (ou .mp3/.wav), ele substitui a
 * música gerada por código.
 */
#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>

typedef enum {
    SFX_SHOOT,          /* tiro do jogador */
    SFX_ENEMY_SHOOT,    /* tiro inimigo */
    SFX_HIT,            /* inimigo atingido (mas não destruído) */
    SFX_EXPLOSION,      /* explosão comum */
    SFX_BIG_EXPLOSION,  /* explosão grande (nave do jogador, chefe) */
    SFX_PLAYER_HIT,     /* jogador perdeu uma vida */
    SFX_SHIELD,         /* escudo absorveu um golpe */
    SFX_POWERUP,        /* power-up coletado */
    SFX_WAVE,           /* início de onda */
    SFX_MENU_MOVE,      /* mover seleção no menu */
    SFX_MENU_SELECT,    /* confirmar no menu */
    SFX_GAME_OVER,      /* jingle de derrota */
    SFX_VICTORY,        /* jingle de vitória */
    SFX_COUNT
} SoundId;

void Audio_Init(void);
void Audio_Unload(void);
void Audio_Update(void);              /* chamar uma vez por frame (alimenta a música) */
void Audio_Play(SoundId id);

void Audio_MusicStart(void);          /* inicia (ou continua) a música, se estiver ligada */
void Audio_MusicStop(void);
void Audio_MusicDuck(bool ducked);    /* true = volume baixo (ex.: no menu de pausa) */
void Audio_MusicToggle(void);         /* tecla M */
bool Audio_MusicEnabled(void);

#endif /* AUDIO_H */
