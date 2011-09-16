#include <ncursesw/curses.h>
#include <jim.h>

Jim_Obj *
JimNCurses_CreateWindow(Jim_Interp *interp, WINDOW *win, char *win_name);

static int
JimNCursesCommand_handler(Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  // return an error if no method is specified
  if (argc < 2) {
    Jim_WrongNumArgs(interp, 1, argv, "method ?args ...?");
  }

  // the ghetto-fabulous command table
  int option;
  static const char * const options[] = {
    "refresh",
    "puts",
    "box",
    "window",
    NULL
  };

  enum {
    OPT_REFRESH,
    OPT_PUTS,
    OPT_BOX,
    OPT_WINDOW
  };

  // figure out which method was called, and pass through the error
  if (
    Jim_GetEnum(interp, argv[1], options, &option, "NCurses method", JIM_ERRMSG) != JIM_OK
  ) {
    return JIM_ERR;
  }

  // shift the arguments to the method context
  argc--; argv++;

  // import the window from the command's private data
  WINDOW *win = Jim_CmdPrivData(interp);

  switch(option) {
  case OPT_REFRESH:
    wrefresh(win);
    break;

  case OPT_PUTS:
    if (argc < 2) {
      Jim_WrongNumArgs(interp, argc, argv, "puts string");
      return JIM_ERR;
    }

    waddstr(win, Jim_String(argv[1]));
    wrefresh(win);
    refresh();
    Jim_SetResult(interp, argv[1]);
    break;

  case OPT_BOX:
    wborder(win, '|', '|', '-', '-', '+', '+', '+', '+');
    break;

  case OPT_WINDOW:
    if (argc < 5) {
      Jim_WrongNumArgs(interp, argc, argv, "height width row column");
      return JIM_ERR;
    }

    // grab the long values of the passed-in dimensions,
    // and give an error if they're malformed
    long dimensions[4];
    int i;
    for (i = 1; i < argc; i++) {
      // awkward offset because argv[0] is the command name
      if(Jim_GetLong(interp, argv[i], dimensions + i - 1) != JIM_OK) {
        Jim_SetResultFormatted(interp,
          "expected an integer but got \"%#s\"", argv[i]
        );
        return JIM_ERR;
      }
    }

    WINDOW *sub = subwin(win,
      dimensions[0], dimensions[1], dimensions[2], dimensions[3]
    );
    Jim_SetResult(interp, JimNCurses_CreateWindow(interp, sub, NULL));
    break;
  }

	return JIM_OK;
}

static void
JimNCurses_DestroyWindow(Jim_Interp *interp, void *privData) {
	// TODO
}

Jim_Obj *
JimNCurses_CreateWindow(Jim_Interp *interp, WINDOW *win, char *win_name) {
  char name[60];
  if (win_name == NULL) {
    win_name = name;
    snprintf(win_name, sizeof(win_name), "ncurses.window<%ld>", Jim_GetId(interp));
  }

  Jim_CreateCommand(
    interp,
    win_name, // the command name
    JimNCursesCommand_handler, // the command itself
    win, // command's private data
    JimNCurses_DestroyWindow // destructor
  );

  return Jim_NewStringObj(interp, win_name, -1);
}

/***
 * native core ncurses commands
 */
// ncurses.init
static int
JimNCursesCommand_init(Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  initscr();
  cbreak();

  JimNCurses_CreateWindow(interp, stdscr, "ncurses.stdscr");

  return JIM_OK;
}

// ncurses.refresh
static int
JimNCursesCommand_refresh(Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  refresh();
  return JIM_OK;
}

// ncurses.end
static int
JimNCursesCommand_end(Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  endwin();
  return JIM_OK;
}

// ncurses.getc
static int
JimNCursesCommand_getc(Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  char ch = getch();
  Jim_SetResultString(interp, &ch, 1);
  return JIM_OK;
}

/**
 * package initializer, run by Jim at load time
 */
int
Jim_ncursesInit(Jim_Interp *interp) {
  if (Jim_PackageProvide(interp, "ncurses", "0.1", JIM_ERRMSG)) {
    return JIM_ERR;
  }

  Jim_CreateCommand(interp, "ncurses.init", JimNCursesCommand_init, NULL, NULL);
  Jim_CreateCommand(interp, "ncurses.end", JimNCursesCommand_end, NULL, NULL);
  Jim_CreateCommand(interp, "ncurses.refresh", JimNCursesCommand_refresh, NULL, NULL);
  Jim_CreateCommand(interp, "ncurses.getc", JimNCursesCommand_getc, NULL, NULL);

	return JIM_OK;
}
