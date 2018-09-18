#ifndef BOARDFMT_H_
#define BOARDFMT_H_

#include "stream.h"
#include "board.h"

extern int move_parse(Move *, Stream *);
extern int pos_parse(Pos *, Stream *);
extern int piece_parse(int *, Stream *);

extern const char *side_name(int);

extern int piece_to_stream(Stream *, int);
extern int move_to_stream(Stream *, const Move *);

extern void board_show(Stream *, const Board *);
extern void board_show_reverse(Stream *, const Board *);
extern void board_show_sfen(Stream *, const Board *, int, int);

#endif /* !BOARDFMT_H_ */
