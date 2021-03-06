default: histarc_build

include ../build/platforms.make

OBJDIR ?= .

histarc_build: $(OBJDIR)/histarc$(EXEEXT)

LDFLAGS += -lsqlite3 -lpthread
CPPFLAGS+=-DHAVE_SQLITE3

SRCS=histarc.c ../cti/String.c ../cti/Mem.c ../cti/Cfg.c ../cti/dbutil.c

$(OBJDIR)/histarc: $(SRCS) Makefile
	$(CC) $(CPPFLAGS) -O $(SRCS) -o $(OBJDIR)/histarc $(LDFLAGS)

install: histarc_build
	cp -v $(OBJDIR)/histarc$(EXEEXT) /platform/$(ARCH)/bin/

clean:
	rm -f $(OBJDIR)/histarc
