lappend auto_path "./lib"
package require ncurses

# bah!  the test proc needs to be in stdlib, guys
source [file dirname [info script]]/../deps/jimtcl/tests/testing.tcl

test ncurses-1.1 "Uninitialized" {
  ncurses.isInitialized
} 0

test ncurses-1.2 "Initialized" {
  try {
    ncurses.init
    set initialized [ncurses.isInitialized]
  } finally {
    ncurses.end
  }

  set initialized
} 1

test ncurses-1.3 "Initialized with ncurses.do" {
  ncurses.do { ncurses.isInitialized }
} 1

test ncurses-1.4 "a window!" {
  ncurses.do {
    set win [stdscr window 10 10 0 0]
  }
  return 0
} 0
