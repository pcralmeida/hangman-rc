# Makefile for hangman-rc
# RC, IST/ULisboa 2022-23
#
# This makefile should be run from the root of the project!!

CC ?= gcc
LD ?= gcc

# space separated list of directories with header files
INCLUDE_DIRS := client . server .
# this creates a space separated list of -I<dir> where <dir> is each of the values in INCLUDE_DIRS
INCLUDES = $(addprefix -I, $(INCLUDE_DIRS))

SOURCES  := $(wildcard */*.c)
HEADERS  := $(wildcard */*.h)
OBJECTS  := $(SOURCES:.c=.o)
TARGET_EXECS := client/client server/server


# VPATH is a variable used by Makefile which finds *sources* and makes them available throughout the codebase
# vpath %.h <DIR> tells make to look for header files in <DIR>
vpath # clears VPATH
vpath %.h $(INCLUDE_DIRS)

CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L
CFLAGS += $(INCLUDES)

# Warnings
CFLAGS += -fdiagnostics-color=always -Wall -Wextra -Wcast-align -Wconversion -Wfloat-equal -Wformat=2 -Wshadow -Wsign-conversion -Wswitch-default -Wswitch-enum -Wundef -Wunreachable-code -Wunused
# Warning suppressions
CFLAGS += -Wno-sign-compare

# optional debug symbols: run make DEBUG=no to deactivate them
ifneq ($(strip $(DEBUG)), no)
  CFLAGS += -g
endif

# optional O3 optimization symbols: run make OPTIM=no to deactivate them
ifeq ($(strip $(OPTIM)), no)
  CFLAGS += -O0
else
  CFLAGS += -O3
endif

.PHONY: all clean depend fmt

all: $(TARGET_EXECS) 
	mv client/client client/player
	mv server/server server/GS


# The following target can be used to invoke clang-format on all the source and header
# files. clang-format is a tool to format the source code based on the style specified.


fmt: $(SOURCES) $(HEADERS)
	clang-format -i $^



# make uses a set of default rules, one of which compiles C binaries
# the CC, LD, CFLAGS and LDFLAGS are used in this rule.
client/client: client/client.o client/client_functions.o

server/server: server/server.o server/server_functions.o

# Does not remove the 'GAMES' folder, because -f can't remove non-empty folders
# exit will be 1 but the result will be the desired one.
clean:
	rm -f $(OBJECTS) $(TARGET_EXECS) client/player server/GS client/*.jpg client/*.jpeg client/[S]* client/[T]* server/SCORES/*.txt server/[GAME_]*
	rm -r server/GAMES/*


# This generates a dependency file, with some default dependencies gathered from the include tree.
# The dependencies are gathered in the file autodep.
depend : $(SOURCES)
	$(CC) $(INCLUDES) -MM $^ > autodep