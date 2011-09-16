# load up the shared lib
load "[file dirname [info script]]/ncurses_ext.so"

proc ncurses.do {script} {
  ncurses.init

  try {
    uplevel 1 $script
  } finally {
    ncurses.end
  }
}

proc stdscr {args} {
  if {[ncurses.isInitialized]} {
    ncurses.stdscr {*}$args
  } else {
    return -code error "stdscr is uninitialized.  Try running your code in `ncurses.do {...}`"
  }
}
