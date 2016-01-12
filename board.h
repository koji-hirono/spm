#ifndef BOARD_H_
#define BOARD_H_

typedef struct Pos Pos;
typedef struct Move Move;
typedef struct Board Board;
typedef struct Movecap Movecap;

enum {
	BLACK,
	WHITE,
	NSIDE
};

enum {
	NONE,
	PAWN,
	LANCE,
	KNIGHT,
	BISHOP,
	ROOK,
	SILVER,
	GOLD,
	KING,
	NPIECE
};

enum {
	HUMAN,
	COM
};

enum {
	MOVE,
	RESIGN,
	WIN,
	NOMATE
};

struct Pos {
	int row;
	int col;
};

struct Move {
	int flags;
	int piece;
	int get_piece;
	Pos src;
	Pos dst;
};

struct Board {
	int side;
	int nrow;
	int ncol;
	int **cell;
	int num[NSIDE][NPIECE];
};

struct Movecap {
	int flags;
	int depth;
	int seldepth;
	int score;
	Move expect;
};

/*
 * Move::flags
 *
 *         1 0
 *   -----+-+-+
 * ...    | | |
 *   -----+-+-+
 *         ^ ^
 *         | |
 *         | +-- 0: on board     1: in hand
 *         +---- 0: not promote  1: promote
 */
#define onboard(m)       (((m)->flags & (1 << 0)) == 0)
#define dopromote(m)     ((m)->flags & (1 << 1))
#define fromstock_set(m) ((m)->flags |= (1 << 0))
#define dopromote_set(m) ((m)->flags |= (1 << 1))

/*
 * Move::piece, get_piece
 *
 *        5 4       0
 *   ----+-+-+-+-+-+-+
 * ...   | | |       |
 *   ----+-+-+-+-+-+-+
 *        ^ ^    ^
 *        | |    |
 *        | |    +---- piece type (none ~ king)
 *        | +--------- 0: not promoted  1: promoted
 *        +----------- 0: black side    1: white side
 */
#define ispromoted(p)    (((p) >> 4) & 1)
#define demote(p)        ((p) & ~(1 << 4))
#define promote(p)       ((p) | (1 << 4))
#define side_get(p)      (((p) >> 5) & 1)
#define side_set(p, s)   ((p) | (((s) & 1) << 5))
#define type_get(p)      ((p) & 0x1f)
#define type_set(p, t)   ((p) | ((t) & 0x1f))
#define side_opponent(s) (!(s))

#define B(p) side_set(p, BLACK)
#define W(p) side_set(p, WHITE)

/* Movecap::flags */
#define MOVECAP_VALID    (1 << 0)


extern int board_init(Board *);
extern int board55_init(Board *);
extern int board_sfen_init(Board *, const char *);
extern int board_alloc(Board *, int, int);
extern void board_destroy(Board *);
extern int board_clone(Board *, const Board *);
extern int *board_cell(const Board *, int, int);

extern void movecap_init(Movecap *);

#endif /* !BOARD_H_ */
