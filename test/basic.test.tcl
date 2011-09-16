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
} "ncurses.window<1>"

test ncurses-1.5 "an out-of-bounds window" {
  catch {
    ncurses.do {
      stdscr window 1000 1000 0 0
    }
  } e

  string match "*out of range*" $e
} 1

testreport
