CFLAGS += -Wall -g
SH_FLAGS += -fpic -shared
LDLIBS += -lncursesw
LDLIBS += -L./deps/jimtcl -I./deps/jimtcl -ljim

JIM = deps/jimtcl/jim.o
JIMSRC = deps/jimtcl/*.c deps/jimtcl/*.h
OBJ = lib/ncurses_ext.so
SRC = src/jim-ncurses.c
JIMSH = ./deps/jimtcl/jimsh
TESTS = ./test/basic.test.tcl

$(OBJ): $(JIM) $(SRC)
	$(CC) $(LDLIBS) $(SH_FLAGS) $(CFLAGS) -o $(OBJ) $(SRC)

$(JIM): $(JIMSRC)
	cd deps/jimtcl && ./configure && make && cd -

jim: $(JIM)

all: $(OBJ)

clean:
	rm $(OBJ)

.PHONY: test
test: all
	$(JIMSH) $(TESTS)

.PHONY: debug
debug: all
	gdb --args $(JIMSH) $(TESTS)

.PHONY: grind
grind: all
	valgrind --leak-check=full $(JIMSH) $(TESTS)
