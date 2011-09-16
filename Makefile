CFLAGS += -Wall -g
SH_FLAGS += -fpic -shared
LDLIBS += -lncursesw
LDLIBS += -L./deps/jimtcl -I./deps/jimtcl -ljim

JIM = deps/jimtcl/jim.o
JIMSRC = deps/jimtcl/*.c deps/jimtcl/*.h
OBJ = lib/ncurses.so
SRC = src/jim-ncurses.c

$(OBJ): $(JIM) $(SRC)
	$(CC) $(LDLIBS) $(SH_FLAGS) $(CFLAGS) -o $(OBJ) $(SRC)

$(JIM): $(JIMSRC)
	cd deps/jimtcl && ./configure && make && cd -

jim: $(JIM)

all: $(OBJ)

clean:
	rm $(OBJ)

test: all
	./deps/jimtcl/jimsh ./test/basic.test.tcl
