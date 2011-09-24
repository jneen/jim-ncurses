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

# test ncurses-1.4 "a window!" {
#   ncurses.do {
#     set win [stdscr window 20 20 20 20]
#     $win box
#     $win puts {3 3} "This should be a box"
#     ncurses.getc
#   }
# 
#   set win
# } "ncurses.window<1>"

test ncurses-1.5 "an out-of-bounds window" {
  catch {
    ncurses.do {
      stdscr window 1000 1000 0 0
    }
  } e

  string match "*out of range*" $e
} 1

test ncurses-1.6 "getmaxyx" {
  ncurses.do {
    set win [stdscr window 3 4 5 6]
    set result [$win getmaxyx]
  }

  set result
} {3 4}

test ncurses-1.7 "height and width" {
  ncurses.do {
    set win [stdscr window 3 4 5 6]
    $win box
    set result [list [$win height] [$win width]]
  }

  set result
} {3 4}

test ncurses-1.8 "getc" {
  ncurses.do {
    foreach x {1 2 3 4} {
      stdscr puts [list $x 0] "entered: [stdscr getc]\n"
    }
    stdscr getc
  }

  list 0
} 0

test ncurses-1.9 "moving the cursor" {
  ncurses.do {
    stdscr puts "centered cursor:\n"
    set width [stdscr width]
    set height [stdscr height]
    stdscr puts "height: $height, width: $width"
    stdscr move [/ $height 2] [/ $width 2]
    stdscr getc
  }

  list 0
} 0

testreport
