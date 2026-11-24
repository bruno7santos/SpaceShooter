/*
 * audio.c — Síntese procedural de sons e música
 * ------------------------------------------------------------------
 * Em vez de carregar arquivos, este módulo "desenha" cada som amostra
 * por amostra (como um sintetizador simples) e entrega o resultado à
 * raylib por meio da struct Wave. Assim o jogo não depende de nenhum
 * asset externo e não há problemas de licença para os sons.
 */
#include "audio.h"

#include <math.h>
#include <string.h>
#include "raylib.h"

#define SAMPLE_RATE   22050     /* taxa de amostragem dos efeitos (Hz) */
#define SFX_COPIES    3         /* cópias de cada efeito: permitem sons sobrepostos */
#define MUSIC_VOLUME  0.35f
#define MUSIC_DUCKED  0.12f

typedef enum { SHAPE_SINE, SHAPE_SQUARE, SHAPE_SAW, SHAPE_TRIANGLE, SHAPE_NOISE } Shape;

/* Um "tom": forma de onda com frequência que desliza de freqStart até freqEnd. */
typedef struct {
    Shape shape;
    float freqStart;     /* Hz */
    float freqEnd;       /* Hz (igual a freqStart = nota fixa) */
    float duration;      /* segundos */
    float volume;        /* volume relativo dentro do efeito */
    float noise;         /* 0..1: quanto de ruído é misturado */
    float smoothStart;   /* filtro passa-baixa (0 = desligado; menor = mais abafado) */
    float smoothEnd;
} Tone;

/* ------------------------------------------------------------------ */
/*  Definição dos efeitos (cada efeito é uma sequência de tons)        */
/* ------------------------------------------------------------------ */

static const Tone T_SHOOT[]     = { { SHAPE_SQUARE,   1000.0f, 280.0f, 0.12f, 1.0f, 0.00f, 0.00f, 0.00f } };
static const Tone T_ESHOOT[]    = { { SHAPE_SAW,       480.0f, 160.0f, 0.18f, 1.0f, 0.10f, 0.00f, 0.00f } };
static const Tone T_HIT[]       = { { SHAPE_TRIANGLE,  260.0f, 110.0f, 0.07f, 1.0f, 0.50f, 0.00f, 0.00f } };
static const Tone T_BOOM[]      = { { SHAPE_NOISE,     100.0f, 100.0f, 0.45f, 1.0f, 1.00f, 0.50f, 0.04f } };
static const Tone T_BIGBOOM[]   = { { SHAPE_SINE,       70.0f,  30.0f, 1.10f, 1.0f, 0.70f, 0.30f, 0.03f } };
static const Tone T_PLAYERHIT[] = { { SHAPE_SAW,       300.0f,  55.0f, 0.40f, 1.0f, 0.35f, 0.00f, 0.00f } };
static const Tone T_SHIELD[]    = { { SHAPE_SINE,      900.0f, 1500.0f, 0.22f, 1.0f, 0.10f, 0.00f, 0.00f } };
static const Tone T_POWERUP[]   = {
    { SHAPE_SQUARE, 523.0f, 523.0f, 0.07f, 1.0f, 0.0f, 0.0f, 0.0f },
    { SHAPE_SQUARE, 659.0f, 659.0f, 0.07f, 1.0f, 0.0f, 0.0f, 0.0f },
    { SHAPE_SQUARE, 784.0f, 784.0f, 0.07f, 1.0f, 0.0f, 0.0f, 0.0f },
    { SHAPE_SQUARE, 1047.0f, 1047.0f, 0.14f, 1.0f, 0.0f, 0.0f, 0.0f }
};
static const Tone T_WAVE[]      = {
    { SHAPE_SQUARE, 392.0f, 392.0f, 0.12f, 1.0f, 0.0f, 0.0f, 0.0f },
    { SHAPE_SQUARE, 523.0f, 523.0f, 0.12f, 1.0f, 0.0f, 0.0f, 0.0f },
    { SHAPE_SQUARE, 659.0f, 659.0f, 0.12f, 1.0f, 0.0f, 0.0f, 0.0f },
    { SHAPE_SQUARE, 784.0f, 784.0f, 0.34f, 1.0f, 0.0f, 0.0f, 0.0f }
};
static const Tone T_MENUMOVE[]  = { { SHAPE_SQUARE,    700.0f, 700.0f, 0.045f, 1.0f, 0.0f, 0.0f, 0.0f } };
static const Tone T_MENUSEL[]   = {
    { SHAPE_SQUARE, 520.0f, 520.0f, 0.06f, 1.0f, 0.0f, 0.0f, 0.0f },
    { SHAPE_SQUARE, 880.0f, 880.0f, 0.10f, 1.0f, 0.0f, 0.0f, 0.0f }
};
static const Tone T_GAMEOVER[]  = {
    { SHAPE_TRIANGLE, 392.0f, 392.0f, 0.28f, 1.0f, 0.0f, 0.0f, 0.0f },
    { SHAPE_TRIANGLE, 330.0f, 330.0f, 0.28f, 1.0f, 0.0f, 0.0f, 0.0f },
    { SHAPE_TRIANGLE, 262.0f, 262.0f, 0.28f, 1.0f, 0.0f, 0.0f, 0.0f },
    { SHAPE_TRIANGLE, 196.0f, 196.0f, 0.70f, 1.0f, 0.0f, 0.0f, 0.0f }
};
static const Tone T_VICTORY[]   = {
    { SHAPE_SQUARE,  523.0f,  523.0f, 0.14f, 1.0f, 0.0f, 0.0f, 0.0f },
    { SHAPE_SQUARE,  659.0f,  659.0f, 0.14f, 1.0f, 0.0f, 0.0f, 0.0f },
    { SHAPE_SQUARE,  784.0f,  784.0f, 0.14f, 1.0f, 0.0f, 0.0f, 0.0f },
    { SHAPE_SQUARE, 1047.0f, 1047.0f, 0.40f, 1.0f, 0.0f, 0.0f, 0.0f },
    { SHAPE_SQUARE,  784.0f,  784.0f, 0.12f, 1.0f, 0.0f, 0.0f, 0.0f },
    { SHAPE_SQUARE, 1047.0f, 1047.0f, 0.70f, 1.0f, 0.0f, 0.0f, 0.0f }
};

typedef struct {
    const Tone *tones;
    int   count;
    float peak;          /* volume máximo do efeito (0..1) — o módulo normaliza para isso */
    float pitchJitter;   /* variação aleatória de altura a cada reprodução */
} SfxDef;

#define SFX(arr, peak, jitter)  { arr, (int)(sizeof(arr) / sizeof((arr)[0])), peak, jitter }

static const SfxDef SFX_DEFS[SFX_COUNT] = {
    [SFX_SHOOT]         = SFX(T_SHOOT,      0.30f, 0.06f),
    [SFX_ENEMY_SHOOT]   = SFX(T_ESHOOT,     0.28f, 0.05f),
    [SFX_HIT]           = SFX(T_HIT,        0.30f, 0.10f),
    [SFX_EXPLOSION]     = SFX(T_BOOM,       0.65f, 0.12f),
    [SFX_BIG_EXPLOSION] = SFX(T_BIGBOOM,    0.90f, 0.00f),
    [SFX_PLAYER_HIT]    = SFX(T_PLAYERHIT,  0.70f, 0.00f),
    [SFX_SHIELD]        = SFX(T_SHIELD,     0.45f, 0.00f),
    [SFX_POWERUP]       = SFX(T_POWERUP,    0.40f, 0.00f),
    [SFX_WAVE]          = SFX(T_WAVE,       0.45f, 0.00f),
    [SFX_MENU_MOVE]     = SFX(T_MENUMOVE,   0.25f, 0.00f),
    [SFX_MENU_SELECT]   = SFX(T_MENUSEL,    0.35f, 0.00f),
    [SFX_GAME_OVER]     = SFX(T_GAMEOVER,   0.55f, 0.00f),
    [SFX_VICTORY]       = SFX(T_VICTORY,    0.55f, 0.00f)
};

/* ------------------------------------------------------------------ */
/*  Estado privado do módulo                                           */
/* ------------------------------------------------------------------ */

static bool           gReady = false;
static Sound          gSfx[SFX_COUNT][SFX_COPIES];
static int            gNext[SFX_COUNT];

static Music          gMusic;
static bool           gMusicLoaded = false;
static bool           gMusicEnabled = true;
static bool           gMusicDucked = false;
static unsigned char *gMusicData = NULL;   /* a raylib lê a música direto desta memória */

/* ------------------------------------------------------------------ */
/*  Síntese                                                            */
/* ------------------------------------------------------------------ */

/* Ruído branco simples (xorshift), independente do gerador do jogo. */
static unsigned int gNoiseState = 22222u;

static float NextNoise(void)
{
    gNoiseState ^= gNoiseState << 13;
    gNoiseState ^= gNoiseState >> 17;
    gNoiseState ^= gNoiseState << 5;
    return ((float)(gNoiseState & 0xFFFFu) / 32767.5f) - 1.0f;   /* -1 .. +1 */
}

/* Valor da onda (-1..+1) para uma fase de 0 a 1. */
static float WaveValue(Shape shape, float phase)
{
    switch (shape) {
        case SHAPE_SINE:     return sinf(phase * 2.0f * PI);
        case SHAPE_SQUARE:   return (phase < 0.5f) ? 1.0f : -1.0f;
        case SHAPE_SAW:      return 2.0f * phase - 1.0f;
        case SHAPE_TRIANGLE: return 4.0f * fabsf(phase - 0.5f) - 1.0f;
        case SHAPE_NOISE:    return NextNoise();
    }
    return 0.0f;
}

/* Gera "frames" amostras de 16 bits para um tom. */
static void Synthesize(short *out, int frames, const Tone *t)
{
    const int attack = 90;    /* ~4 ms de entrada suave: evita "estalos" */
    float phase = 0.0f;
    float filtered = 0.0f;

    for (int i = 0; i < frames; i++) {
        float progress = (float)i / (float)frames;                      /* 0 -> 1 */
        float freq = t->freqStart * powf(t->freqEnd / t->freqStart, progress);

        phase += freq / (float)SAMPLE_RATE;
        if (phase >= 1.0f) phase -= 1.0f;

        float s = WaveValue(t->shape, phase);
        if (t->noise > 0.0f) s = s * (1.0f - t->noise) + NextNoise() * t->noise;

        if (t->smoothStart > 0.0f) {                                   /* passa-baixa de 1 polo */
            float k = t->smoothStart + (t->smoothEnd - t->smoothStart) * progress;
            filtered += k * (s - filtered);
            s = filtered;
        }

        float env = (1.0f - progress);                                 /* decaimento */
        env *= env;
        if (i < attack) env *= (float)i / (float)attack;

        out[i] = (short)(s * env * t->volume * 32767.0f * 0.5f);
    }
}

static void BuildSfx(SoundId id)
{
    const SfxDef *def = &SFX_DEFS[id];
    int total = 0;

    for (int i = 0; i < def->count; i++) total += (int)(def->tones[i].duration * (float)SAMPLE_RATE);
    if (total <= 0) return;

    short *pcm = (short *)MemAlloc((unsigned int)total * (unsigned int)sizeof(short));
    if (pcm == NULL) return;

    int offset = 0;
    for (int i = 0; i < def->count; i++) {
        int frames = (int)(def->tones[i].duration * (float)SAMPLE_RATE);
        Synthesize(pcm + offset, frames, &def->tones[i]);
        offset += frames;
    }

    /* Normaliza: o pico do efeito passa a ser exatamente def->peak. */
    int maxAbs = 1;
    for (int i = 0; i < total; i++) {
        int v = pcm[i] < 0 ? -pcm[i] : pcm[i];
        if (v > maxAbs) maxAbs = v;
    }
    float gain = (def->peak * 32767.0f) / (float)maxAbs;
    for (int i = 0; i < total; i++) pcm[i] = (short)((float)pcm[i] * gain);

    Wave wave;
    memset(&wave, 0, sizeof wave);
    wave.frameCount = (unsigned int)total;
    wave.sampleRate = SAMPLE_RATE;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = pcm;

    for (int c = 0; c < SFX_COPIES; c++) gSfx[id][c] = LoadSoundFromWave(wave);

    UnloadWave(wave);   /* a raylib já copiou os dados para o buffer de áudio */
}

/* ------------------------------------------------------------------ */
/*  Música gerada por código                                           */
/* ------------------------------------------------------------------ */

#define MUSIC_RATE   22050
#define MUSIC_BPM    112.0f
#define MUSIC_STEPS  128      /* 8 compassos x 16 semicolcheias */

typedef struct { int bass; int arp[4]; } Chord;   /* notas em número MIDI (69 = Lá 440 Hz) */

static const Chord CHORDS[4] = {
    { 45, { 57, 60, 64, 69 } },   /* Lá menor  */
    { 41, { 53, 57, 60, 65 } },   /* Fá maior  */
    { 48, { 60, 64, 67, 72 } },   /* Dó maior  */
    { 43, { 55, 59, 62, 67 } }    /* Sol maior */
};

static const int ARP_PATTERN[16] = { 0, 1, 2, 3, 2, 1, 2, 3, 0, 1, 2, 3, 2, 1, 3, 2 };

static float NoteHz(int midi)
{
    return 440.0f * powf(2.0f, ((float)midi - 69.0f) / 12.0f);
}

/* Soma uma nota ao buffer de mixagem. O índice dá a volta (%) para o loop ficar sem emendas. */
static void MixNote(float *mix, int total, int start, float hz, float seconds,
                    float volume, Shape shape, float decay)
{
    int frames = (int)(seconds * (float)MUSIC_RATE);
    float phase = 0.0f;

    for (int i = 0; i < frames; i++) {
        float progress = (float)i / (float)frames;
        float env = powf(1.0f - progress, decay);
        if (i < 110) env *= (float)i / 110.0f;
        phase += hz / (float)MUSIC_RATE;
        if (phase >= 1.0f) phase -= 1.0f;
        mix[(start + i) % total] += WaveValue(shape, phase) * env * volume;
    }
}

static void MixKick(float *mix, int total, int start)
{
    int frames = (int)(0.16f * (float)MUSIC_RATE);
    float phase = 0.0f;

    for (int i = 0; i < frames; i++) {
        float progress = (float)i / (float)frames;
        float hz = 140.0f * powf(45.0f / 140.0f, progress);
        phase += hz / (float)MUSIC_RATE;
        mix[(start + i) % total] += sinf(phase * 2.0f * PI) * powf(1.0f - progress, 2.0f) * 0.28f;
    }
}

static void MixHat(float *mix, int total, int start)
{
    int frames = (int)(0.05f * (float)MUSIC_RATE);
    float previous = 0.0f;

    for (int i = 0; i < frames; i++) {
        float progress = (float)i / (float)frames;
        float n = NextNoise();
        float highpassed = n - previous;          /* diferença entre amostras = filtro passa-alta */
        previous = n;
        mix[(start + i) % total] += highpassed * powf(1.0f - progress, 3.0f) * 0.07f;
    }
}

/* Nota longa e suave (pad) para dar "corpo" ao acorde. */
static void MixPad(float *mix, int total, int start, int frames, int midi, float volume)
{
    int attack = (int)(0.5f * (float)MUSIC_RATE);
    int release = (int)(0.6f * (float)MUSIC_RATE);
    float hz = NoteHz(midi);
    float phase = 0.0f;

    for (int i = 0; i < frames; i++) {
        float env = 1.0f;
        if (i < attack) env = (float)i / (float)attack;
        if (frames - i < release) {
            float r = (float)(frames - i) / (float)release;
            if (r < env) env = r;
        }
        phase += hz / (float)MUSIC_RATE;
        if (phase >= 1.0f) phase -= 1.0f;
        mix[(start + i) % total] += WaveValue(SHAPE_TRIANGLE, phase) * env * volume;
    }
}

static void PutU16(unsigned char *p, unsigned int v)
{
    p[0] = (unsigned char)(v & 0xFFu);
    p[1] = (unsigned char)((v >> 8) & 0xFFu);
}

static void PutU32(unsigned char *p, unsigned int v)
{
    PutU16(p, v & 0xFFFFu);
    PutU16(p + 2, (v >> 16) & 0xFFFFu);
}

/*
 * Compõe o loop da música e devolve um arquivo WAV completo na memória
 * (cabeçalho de 44 bytes + amostras de 16 bits), pronto para
 * LoadMusicStreamFromMemory. O chamador libera com MemFree.
 */
static unsigned char *BuildMusicWav(int *outSize)
{
    float stepFrames = (60.0f / MUSIC_BPM / 4.0f) * (float)MUSIC_RATE;
    int total = (int)(stepFrames * (float)MUSIC_STEPS);

    float *mix = (float *)MemAlloc((unsigned int)total * (unsigned int)sizeof(float));
    if (mix == NULL) return NULL;

    for (int step = 0; step < MUSIC_STEPS; step++) {
        int start = (int)((float)step * stepFrames);
        const Chord *chord = &CHORDS[step / 32];
        int inBar = step % 16;

        /* baixo em colcheias, com acento no início de cada tempo */
        if (step % 2 == 0) {
            MixNote(mix, total, start, NoteHz(chord->bass), 0.24f,
                    (inBar % 4 == 0) ? 0.15f : 0.10f, SHAPE_TRIANGLE, 1.6f);
        }
        /* arpejo em semicolcheias */
        MixNote(mix, total, start, NoteHz(chord->arp[ARP_PATTERN[inBar]]), 0.20f,
                0.11f, SHAPE_SQUARE, 2.5f);
        /* bumbo nos tempos 1 e 3, chimbal entre os tempos */
        if (inBar == 0 || inBar == 8) MixKick(mix, total, start);
        if (inBar % 4 == 2) MixHat(mix, total, start);
        /* pad: três notas do acorde, uma oitava abaixo, a cada 2 compassos */
        if (step % 32 == 0) {
            for (int k = 0; k < 3; k++) {
                MixPad(mix, total, start, (int)(32.0f * stepFrames), chord->arp[k] - 12, 0.03f);
            }
        }
    }

    /* normaliza o pico para 0,8 */
    float peak = 0.0001f;
    for (int i = 0; i < total; i++) {
        float a = fabsf(mix[i]);
        if (a > peak) peak = a;
    }
    float gain = 0.8f / peak;

    int dataBytes = total * 2;
    int fileBytes = 44 + dataBytes;
    unsigned char *wav = (unsigned char *)MemAlloc((unsigned int)fileBytes);
    if (wav == NULL) {
        MemFree(mix);
        return NULL;
    }

    /* cabeçalho WAV (PCM, mono, 16 bits) */
    memcpy(wav + 0, "RIFF", 4);
    PutU32(wav + 4, (unsigned int)(36 + dataBytes));
    memcpy(wav + 8, "WAVE", 4);
    memcpy(wav + 12, "fmt ", 4);
    PutU32(wav + 16, 16);                     /* tamanho do bloco fmt */
    PutU16(wav + 20, 1);                      /* 1 = PCM */
    PutU16(wav + 22, 1);                      /* canais */
    PutU32(wav + 24, MUSIC_RATE);             /* taxa de amostragem */
    PutU32(wav + 28, MUSIC_RATE * 2);         /* bytes por segundo */
    PutU16(wav + 32, 2);                      /* bytes por frame */
    PutU16(wav + 34, 16);                     /* bits por amostra */
    memcpy(wav + 36, "data", 4);
    PutU32(wav + 40, (unsigned int)dataBytes);

    for (int i = 0; i < total; i++) {
        short s = (short)(mix[i] * gain * 32767.0f);
        PutU16(wav + 44 + i * 2, (unsigned int)(unsigned short)s);
    }

    MemFree(mix);
    *outSize = fileBytes;
    return wav;
}

static void ApplyMusicVolume(void)
{
    if (gMusicLoaded) SetMusicVolume(gMusic, gMusicDucked ? MUSIC_DUCKED : MUSIC_VOLUME);
}

static void LoadMusicTrack(void)
{
    static const char *files[] = {
        "assets/sounds/music.ogg", "assets/sounds/music.mp3", "assets/sounds/music.wav"
    };

    /* 1) música externa opcional (coloque o arquivo e registre o crédito no README) */
    for (int i = 0; i < 3; i++) {
        if (!FileExists(files[i])) continue;
        gMusic = LoadMusicStream(files[i]);
        if (gMusic.stream.buffer != NULL) {
            gMusicLoaded = true;
            ApplyMusicVolume();
            return;
        }
    }

    /* 2) trilha gerada por código */
    int size = 0;
    gMusicData = BuildMusicWav(&size);
    if (gMusicData == NULL) return;

    gMusic = LoadMusicStreamFromMemory(".wav", gMusicData, size);
    if (gMusic.stream.buffer != NULL) {
        gMusicLoaded = true;
        ApplyMusicVolume();
    }
}

/* ------------------------------------------------------------------ */
/*  API pública                                                        */
/* ------------------------------------------------------------------ */

void Audio_Init(void)
{
    InitAudioDevice();
    gReady = IsAudioDeviceReady();
    if (!gReady) return;               /* sem placa de som: o jogo roda em silêncio */

    SetMasterVolume(0.85f);
    for (int i = 0; i < SFX_COUNT; i++) BuildSfx((SoundId)i);
    LoadMusicTrack();
}

void Audio_Unload(void)
{
    if (!gReady) return;

    for (int i = 0; i < SFX_COUNT; i++) {
        for (int c = 0; c < SFX_COPIES; c++) UnloadSound(gSfx[i][c]);
    }
    if (gMusicLoaded) {
        UnloadMusicStream(gMusic);
        gMusicLoaded = false;
    }
    if (gMusicData != NULL) {          /* só pode liberar depois de descarregar a música */
        MemFree(gMusicData);
        gMusicData = NULL;
    }
    CloseAudioDevice();
    gReady = false;
}

void Audio_Update(void)
{
    if (gReady && gMusicLoaded) UpdateMusicStream(gMusic);
}

void Audio_Play(SoundId id)
{
    if (!gReady || (int)id < 0 || (int)id >= SFX_COUNT) return;

    Sound s = gSfx[id][gNext[id]];
    gNext[id] = (gNext[id] + 1) % SFX_COPIES;

    float jitter = SFX_DEFS[id].pitchJitter;
    if (jitter > 0.0f) {
        SetSoundPitch(s, 1.0f + ((float)GetRandomValue(-100, 100) / 100.0f) * jitter);
    }
    PlaySound(s);
}

void Audio_MusicStart(void)
{
    if (!gReady || !gMusicLoaded || !gMusicEnabled) return;
    if (!IsMusicStreamPlaying(gMusic)) PlayMusicStream(gMusic);
    ApplyMusicVolume();
}

void Audio_MusicStop(void)
{
    if (gReady && gMusicLoaded) StopMusicStream(gMusic);
}

void Audio_MusicDuck(bool ducked)
{
    gMusicDucked = ducked;
    ApplyMusicVolume();
}

void Audio_MusicToggle(void)
{
    gMusicEnabled = !gMusicEnabled;
    if (gMusicEnabled) Audio_MusicStart();
    else Audio_MusicStop();
}

bool Audio_MusicEnabled(void)
{
    return gMusicEnabled;
}
