/*
 * collision.h — Detecção de colisões
 */
#ifndef COLLISION_H
#define COLLISION_H

struct Game;   /* declaração antecipada (evita include circular com game.h) */

/*
 * Testa todas as colisões do frame e avisa o jogo por meio dos "eventos"
 * Game_OnEnemyHit, Game_OnPlayerHit etc.:
 *   1) tiros do jogador  x inimigos
 *   2) inimigos          x jogador
 *   3) tiros inimigos    x jogador
 *   4) power-ups         x jogador
 */
void Collision_Update(struct Game *game);

#endif /* COLLISION_H */
