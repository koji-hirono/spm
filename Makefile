PROG=spm
SRCS=\
	main.c \
	interp.c \
	cmds.c \
	engine.c \
	usi.c \
	game.c \
	board.c \
	move.c \
	isreach.c \
	boardfmt.c \
	movelog.c \
	stream.c \
	buf.c \
	trace.c \
	stringex.c

OBJS=$(SRCS:.c=.o)
DEPS=$(SRCS:.c=.dep)
CPPFLAGS=-Wall -W -Werror
CPPFLAGS+=-Wno-unused-parameter

.PHONY: all clean

all: $(PROG)

clean:
	-rm -rf $(DEPS) $(OBJS) $(PROG) *~

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDADD)

$(OBJS): %.o : %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(DEPS): %.dep : %.c
	@echo "===> Update" $@
	@$(CC) $(CPPFLAGS) -MM $< -o $@

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif
