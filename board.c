#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "trace.h"
#include "board.h"


int
board_alloc(Board *board, int nrow, int ncol)
{
	int **p;
	int row;
	int side;
	int piece;

	if ((p = malloc(nrow * sizeof(int *))) == NULL) {
		error("malloc");
		return -1;
	}

	if ((p[0] = calloc(nrow * ncol, sizeof(int))) == NULL) {
		error("calloc");
		free(p);
		return -1;
	}

	for (row = 1; row < nrow; row++)
		p[row] = p[0] + row * ncol;

	board->side = 0;
	board->nrow = nrow;
	board->ncol = ncol;
	board->cell = p;
	for (side = 0; side < NSIDE; side++)
		for (piece = 0; piece < NPIECE; piece++)
			board->num[side][piece] = 0;

	return 0;
}

void
board_destroy(Board *board)
{
	free(board->cell[0]);
	free(board->cell);
}

int
board_clone(Board *dst, const Board *src)
{
	int row, col;
	int side, piece;

	if (board_alloc(dst, src->nrow, src->ncol) != 0)
		return -1;

	dst->side = src->side;

	for (row = 0; row < src->nrow; row++)
		for (col = 0; col < src->ncol; col++)
			dst->cell[row][col] = src->cell[row][col];

	for (side = 0; side < NSIDE; side++)
		for (piece = 0; piece < NPIECE; piece++)
			dst->num[side][piece] = src->num[side][piece];

	return 0;
}

int
board_init(Board *board)
{
	if (board_alloc(board, 9, 9))
		return -1;

	board->cell[0][0] = W(LANCE);
	board->cell[0][1] = W(KNIGHT);
	board->cell[0][2] = W(SILVER);
	board->cell[0][3] = W(GOLD);
	board->cell[0][4] = W(KING);
	board->cell[0][5] = W(GOLD);
	board->cell[0][6] = W(SILVER);
	board->cell[0][7] = W(KNIGHT);
	board->cell[0][8] = W(LANCE);
	board->cell[1][1] = W(ROOK);
	board->cell[1][7] = W(BISHOP);
	board->cell[2][0] = W(PAWN);
	board->cell[2][1] = W(PAWN);
	board->cell[2][2] = W(PAWN);
	board->cell[2][3] = W(PAWN);
	board->cell[2][4] = W(PAWN);
	board->cell[2][5] = W(PAWN);
	board->cell[2][6] = W(PAWN);
	board->cell[2][7] = W(PAWN);
	board->cell[2][8] = W(PAWN);

	board->cell[6][0] = B(PAWN);
	board->cell[6][1] = B(PAWN);
	board->cell[6][2] = B(PAWN);
	board->cell[6][3] = B(PAWN);
	board->cell[6][4] = B(PAWN);
	board->cell[6][5] = B(PAWN);
	board->cell[6][6] = B(PAWN);
	board->cell[6][7] = B(PAWN);
	board->cell[6][8] = B(PAWN);
	board->cell[7][1] = B(BISHOP);
	board->cell[7][7] = B(ROOK);
	board->cell[8][0] = B(LANCE);
	board->cell[8][1] = B(KNIGHT);
	board->cell[8][2] = B(SILVER);
	board->cell[8][3] = B(GOLD);
	board->cell[8][4] = B(KING);
	board->cell[8][5] = B(GOLD);
	board->cell[8][6] = B(SILVER);
	board->cell[8][7] = B(KNIGHT);
	board->cell[8][8] = B(LANCE);

	board->side = BLACK;

	return 0;
}

int
board55_init(Board *board)
{
	if (board_alloc(board, 5, 5) != 0)
		return -1;

	board->cell[0][0] = W(ROOK);
	board->cell[0][1] = W(BISHOP);
	board->cell[0][2] = W(SILVER);
	board->cell[0][3] = W(GOLD);
	board->cell[0][4] = W(KING);
	board->cell[1][4] = W(PAWN);

	board->cell[3][0] = B(PAWN);
	board->cell[4][0] = B(KING);
	board->cell[4][1] = B(GOLD);
	board->cell[4][2] = B(SILVER);
	board->cell[4][3] = B(BISHOP);
	board->cell[4][4] = B(ROOK);

	board->side = BLACK;

	return 0;
}
int
board_sfen_init(Board *board, const char *s)
{
	const char *p;
	const char *brkp;
	int side;
	int pflag;
	int piece;
	int row;
	int col;
	int n;
	int i;

	if (board_alloc(board, 9, 9))
		return -1;

	/* board */
	row = 0;
	col = 0;
	for (p = s; *p != '\0'; p++) {
		if (*p == ' ')
			break;
		if (*p == '/') {
			col = 0;
			row++;
			continue;
		}
		pflag = 0;
		if (*p == '+') {
			pflag = 1;
			p++;
		}
		if (isdigit(*p & 0xff)) {
			/* space count */
			n = *p - '0';
			piece = NONE;
			for (i = 0; i < n; i++) {
				board->cell[row][col] = piece;
				col++;
			}
		} else if (isupper(*p & 0xff)) {
			switch (*p) {
			case 'P':
				piece = B(PAWN);
				break;
			case 'L':
				piece = B(LANCE);
				break;
			case 'N':
				piece = B(KNIGHT);
				break;
			case 'B':
				piece = B(BISHOP);
				break;
			case 'R':
				piece = B(ROOK);
				break;
			case 'S':
				piece = B(SILVER);
				break;
			case 'G':
				piece = B(GOLD);
				break;
			case 'K':
				piece = B(KING);
				break;
			default:
				piece = NONE;
				break;
			}
			if (pflag)
				piece = promote(piece);
			board->cell[row][col] = piece;
			col++;
		} else if (islower(*p & 0xff)) {
			switch (*p) {
			case 'p':
				piece = W(PAWN);
				break;
			case 'l':
				piece = W(LANCE);
				break;
			case 'n':
				piece = W(KNIGHT);
				break;
			case 'b':
				piece = W(BISHOP);
				break;
			case 'r':
				piece = W(ROOK);
				break;
			case 's':
				piece = W(SILVER);
				break;
			case 'g':
				piece = W(GOLD);
				break;
			case 'k':
				piece = W(KING);
				break;
			default:
				piece = NONE;
				break;
			}
			if (pflag)
				piece = promote(piece);
			board->cell[row][col] = piece;
			col++;
		} else {
			continue;
		}
	}

	while (isspace(*p & 0xff))
		p++;

	/* side */
	if (*p == 'w')
		board->side = WHITE;
	else if (*p == 'b')
		board->side = BLACK;
	p++;

	while (isspace(*p & 0xff))
		p++;

	/* stock */
	for (; *p != '\0'; p++) {
		if (isspace(*p & 0xff))
			break;
		/* stock count */
		n = strtol(p, (char **)&brkp, 10);
		if (n == 0)
			n = 1;
		p = brkp;
		if (isupper(*p & 0xff))
			side = BLACK;
		else if (islower(*p & 0xff))
			side = WHITE;
		else
			continue;
		switch (*p) {
		case 'p':
		case 'P':
			piece = PAWN;
			break;
		case 'l':
		case 'L':
			piece = LANCE;
			break;
		case 'n':
		case 'N':
			piece = KNIGHT;
			break;
		case 'b':
		case 'B':
			piece = BISHOP;
			break;
		case 'r':
		case 'R':
			piece = ROOK;
			break;
		case 's':
		case 'S':
			piece = SILVER;
			break;
		case 'g':
		case 'G':
			piece = GOLD;
			break;
		case 'k':
		case 'K':
			piece = KING;
			break;
		default:
			piece = NONE;
			break;
		}
		board->num[side][piece] = n;
	}

	return 0;
}

int *
board_cell(const Board *board, int row, int col)
{
	return &board->cell[row - 1][board->ncol - col];
}

void
movecap_init(Movecap *cap)
{
	cap->flags = 0;
	cap->score = 0;
	cap->depth = 0;
	cap->seldepth = 0;
}
