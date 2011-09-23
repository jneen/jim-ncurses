# load up the shared lib
load "[file dirname [info script]]/ncurses_ext.so"

####
# runs a script in ncurses mode, and cleans up afterwards
proc ncurses.do {script} {
  ncurses.init

  try {
    uplevel 1 $script
  } finally {
    ncurses.end
  }
}

proc ncurses.window::puts {window {coords {0 0}} str} {
  set row [lindex $coords 0]
  set col [lindex $coords 1]

  $window mvaddstr $row $col $str
}

###
# a facade for ncurses.stdscr, which raises a proper error if
# we're not in ncurses mode
proc stdscr {args} {
  if {[ncurses.isInitialized]} {
    ncurses.stdscr {*}$args
  } else {
    return -code error "stdscr is uninitialized.  Try running your code in `ncurses.do {...}`"
  }
}
