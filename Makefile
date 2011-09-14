CFLAGS += -Wall -g
SH_FLAGS += -fpic -shared
LDLIBS += -lncursesw
LDLIBS += -L./deps/jimtcl -I./deps/jimtcl -ljim

OBJ = lib/ncurses.so
SRC = src/jim-ncurses.c
$(OBJ): $(DEPS) $(SRC)
	$(CC) $(LDLIBS) $(SH_FLAGS) $(CFLAGS) -o $(OBJ) $(SRC)

JIM = deps/jimtcl/jim.o
$(JIM):
	cd deps/jimtcl && ./configure && make && cd -

all: $(OBJ)

clean:
	rm $(OBJ)

test: all
	./test/basic.test.tcl
