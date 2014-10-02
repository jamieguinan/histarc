default: default1

include ../platforms.make

OBJDIR ?= .

default1:  $(OBJDIR)/histarc$(EXEEXT)

LDFLAGS += -lsqlite3
CPPFLAGS+=-DHAVE_SQLITE3

SRCS=histarc.c ../cti/String.c ../cti/Mem.c ../cti/Cfg.c ../cti/Signals.c
$(OBJDIR)/histarc: $(SRCS) Makefile
	$(CC) $(CPPFLAGS) -O $(SRCS) -o $(OBJDIR)/histarc $(LDFLAGS)
