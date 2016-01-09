#ifndef MOVE_H_
#define MOVE_H_

#include "board.h"

extern int isopenway(const Board *, const Pos *, const Pos *);
extern int isillegal_promote(Move *, const Board *);
extern int isillegal_nextmove(Move *, const Board *);
extern int isdouble_pawn(Move *, const Board *);
extern int iskingdead(int, const Board *);
extern int isfoul(Move *, const Board *);
extern int move_normalize(Move *, Board *, int);
extern int move_check(Move *, const Board *);
extern void move_next(const Move *, Board *);
extern void move_prev(const Move *, Board *);

#endif /* !MOVE_H_ */
