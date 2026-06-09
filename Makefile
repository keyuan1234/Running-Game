CC := gcc
APP := running_game.exe
SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)

CFLAGS := -std=c11 -Wall -Wextra -O2
LDFLAGS := -mwindows
LDLIBS := -luser32 -lgdi32 -lwinmm -lmsimg32

.PHONY: all clean run

all: $(APP)

$(APP): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(APP)
	./$(APP)

clean:
	del /Q src\*.o $(APP) 2>NUL || exit 0

