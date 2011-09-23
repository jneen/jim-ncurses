JIM_PATH ?= ./deps/jimtcl
JIM = $(JIM_PATH)/jim.o
JIMSRC = $(JIM_PATH)/*.c $(JIM_PATH)/*.h
OBJ = lib/ncurses_ext.so
SRC = src/jim-ncurses.c
JIMSH = $(JIM_PATH)/jimsh
TESTS = ./test/basic.test.tcl

CFLAGS += -Wall -g
SH_FLAGS += -fpic -shared
LDLIBS += -lncursesw
LDLIBS += -L$(JIM_PATH) -I$(JIM_PATH) -ljim

$(OBJ): $(JIM) $(SRC)
	$(CC) $(LDLIBS) $(SH_FLAGS) $(CFLAGS) -o $(OBJ) $(SRC)

$(JIM): $(JIMSRC)
	cd $(JIM_PATH) && ./configure && make && cd -

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
