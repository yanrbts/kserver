CFLAGS  += -std=c99 -Wall -O2 -D_REENTRANT -DLOG_USE_COLOR -g -I./civetweb/include
LIBS    := ./civetweb/libcivetweb.a -lm -lssl -lpthread

TARGET  := $(shell uname -s | tr '[A-Z]' '[a-z]' 2>/dev/null || echo unknown)

ifeq ($(TARGET), sunos)
	CFLAGS += -D_PTHREADS -D_POSIX_C_SOURCE=200112L
	LIBS   += -lsocket
else ifeq ($(TARGET), darwin)
	export MACOSX_DEPLOYMENT_TARGET = $(shell sw_vers -productVersion)
else ifeq ($(TARGET), linux)
	CFLAGS  += -D_POSIX_C_SOURCE=200112L -D_BSD_SOURCE -D_DEFAULT_SOURCE
	LIBS    += -ldl
	LDFLAGS += -Wl,-E
else ifeq ($(TARGET), freebsd)
	CFLAGS  += -D_DECLARE_C99_LDBL_MATH
	LDFLAGS += -Wl,-E
endif

SRC  := kserver.c zmalloc.c log.c cJSON.c data.c 
		
BIN  := kserver
VER  ?= $(shell git describe --tags --always --dirty)

ODIR := obj
OBJ  := $(patsubst %.c,$(ODIR)/%.o,$(SRC))

DEPS    :=
CFLAGS  += -I$(ODIR)/include
LDFLAGS += -L$(ODIR)/lib


all: $(BIN)

clean:
	$(RM) -rf $(BIN) obj/*

$(BIN): $(OBJ)
	@echo LINK $(BIN)
	@$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OBJ): Makefile $(DEPS) | $(ODIR)

$(ODIR):
	@mkdir -p $@

$(ODIR)/%.o : %.c
	@echo CC $<
	@$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: all clean

.SUFFIXES:
.SUFFIXES: .c .o 

vpath %.c   src
vpath %.h   src
