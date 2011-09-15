# load up the shared lib
load "[file dirname [info script]]/ncurses.so"

proc ncurses.do {script} {
	ncurses.init

	try {
		uplevel 1 [list eval $script]
	} finally {
		ncurses.end
	}
}
