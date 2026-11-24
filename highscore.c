/*
 * highscore.c — Leitura e gravação do recorde
 * O recorde fica em um arquivo de texto simples (highscore.dat), na pasta
 * de trabalho do jogo, e por isso permanece salvo entre execuções.
 */
#define _CRT_SECURE_NO_WARNINGS   /* silencia avisos do MSVC sobre fopen/fscanf */

#include "highscore.h"

#include <stdio.h>
#include "config.h"

int HighScore_Load(void)
{
    int value = 0;
    FILE *file = fopen(HIGHSCORE_FILE, "r");

    if (file == NULL) return 0;                       /* primeira execução */
    if (fscanf(file, "%d", &value) != 1 || value < 0) value = 0;
    fclose(file);
    return value;
}

bool HighScore_Save(int score)
{
    FILE *file = fopen(HIGHSCORE_FILE, "w");

    if (file == NULL) return false;
    fprintf(file, "%d\n", score);
    fclose(file);
    return true;
}
