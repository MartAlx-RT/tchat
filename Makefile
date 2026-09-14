CC = clang
DAEMON_NAME = terminalsd
PROG_NAME = terminals

CFLAGS += -g -Wall -Wextra -Wshadow -fsanitize=address,leak $(IFLAGS)

IFLAGS = -Iinclude
lFLAGS = -lreadline

V :=
ifneq ($(VERBOSE), ON)
	V := @
endif

DAEMON_SOURCES = src/terminalsd.c src/db.c
PROG_SOURCES =  src/db.c src/send_msg.c

DAEMON_OBJECTS = $(DAEMON_SOURCES:src/%.c=build/%.o)
PROG_OBJECTS = $(PROG_SOURCES:src/%.c=build/%.o)

DEPS = $(DAEMON_OBJECTS:.o=.d) $(PROG_OBJECTS:.o=.d)

all:	$(PROG_NAME) $(DAEMON_NAME) | Makefile

$(PROG_NAME):	$(PROG_OBJECTS)
	@echo [Linking]
	$V $(CC) -o $@ $(CFLAGS) $(lFLAGS) $^
	@echo [Done.]

$(DAEMON_NAME):	$(DAEMON_OBJECTS)
	@echo [Linking]
	$V $(CC) -o $@ $(CFLAGS) $(lFLAGS) $^
	@echo [Done.]

build/%.o:	src/%.c | build
	@echo [Building CXX]
	$V $(CC) -c -MMD -MP -o $@ $(CFLAGS) $<

build:
	$V mkdir -p $@

clean:
	$V rm -rf build $(PROG_NAME) $(DAEMON_NAME)

Makefile:
	@echo [Makefile has been changed, rebuilding]
	@make clean

.PHONY:	clean all

-include $(DEPS)
