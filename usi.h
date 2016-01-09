#ifndef USI_H_
#define USI_H_

typedef struct Token Token;
typedef struct Usiobj Usiobj;
typedef struct Usimsg Usimsg;

/* message type */
enum {
	USIMSG_ID,
	USIMSG_OPTION,
	USIMSG_OK,
	USIMSG_READYOK,
	USIMSG_INFO,
	USIMSG_BESTMOVE,
	USIMSG_CHECKMATE
};

/* object key */
enum {
	USIOBJ_PROGNAME,
	USIOBJ_AUTHOR,
	USIOBJ_DEPTH,
	USIOBJ_SELDEPTH,
	USIOBJ_TIME,
	USIOBJ_NODES,
	USIOBJ_PV,
	USIOBJ_SCORE_CP,
	USIOBJ_SCORE_MATE,
	USIOBJ_CURRMOVE,
	USIOBJ_HASHFULL,
	USIOBJ_NPS,
	USIOBJ_STRING,
	USIOBJ_OPTNAME,
	USIOBJ_TYPE,
	USIOBJ_DEFAULT,
	USIOBJ_MIN,
	USIOBJ_MAX,
	USIOBJ_VAR,
	USIOBJ_MOVE,
	USIOBJ_RESIGN,
	USIOBJ_WIN,
	USIOBJ_NOTIMPLEMENTED,
	USIOBJ_TIMEOUT,
	USIOBJ_NOMATE
};

/* option type */
enum {
	USIOPT_CHECK,
	USIOPT_SPIN,
	USIOPT_COMBO,
	USIOPT_BUTTON,
	USIOPT_STRING,
	USIOPT_FILENAME
};

/* object type */
enum {
	USITYPE_NIL,
	USITYPE_NUM,
	USITYPE_STR,
	USITYPE_OCT
};

struct Token {
	const char *s;
	size_t len;
};

struct Usiobj {
	Usiobj *next;
	int key;
};

struct Usimsg {
	int type;
	Usiobj *head;
	Usiobj *tail;
};

extern int usimsg_parse(Usimsg *, const char *);
extern void usimsg_destroy(Usimsg *);
extern Usiobj *usiobj_find(Usimsg *, int);
extern const char *usiopttypestr(int);

#endif /* !USI_H_ */
