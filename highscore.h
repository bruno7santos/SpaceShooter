/*
 * highscore.h — Recorde (High Score) salvo em arquivo
 */
#ifndef HIGHSCORE_H
#define HIGHSCORE_H

#include <stdbool.h>

int  HighScore_Load(void);              /* devolve 0 se o arquivo não existir ou for inválido */
bool HighScore_Save(int score);         /* devolve false se não conseguir gravar */

#endif /* HIGHSCORE_H */
