#include <string.h>
#include <ncursesw/curses.h>
#include <jim.h>

/************ prototypes ************/

// window command handler
static int
JimNCursesCommand_handler(Jim_Interp *interp, int argc, Jim_Obj *const *argv);

// static commands
static int
JimNCursesCommand_init(Jim_Interp *interp, int argc, Jim_Obj *const *argv);
static int
JimNCursesCommand_isInitialized(Jim_Interp *interp, int argc, Jim_Obj *const *argv);
static int
JimNCursesCommand_end(Jim_Interp *interp, int argc, Jim_Obj *const *argv);
static int
JimNCursesCommand_refresh(Jim_Interp *interp, int argc, Jim_Obj *const *argv);
static int
JimNCursesCommand_getc(Jim_Interp *interp, int argc, Jim_Obj *const *argv);

// helpers
static void
JimNCurses_DestroyWindow(Jim_Interp *interp, void *privData);
static void
JimNCurses_CreateWindow(Jim_Interp *interp, WINDOW *win, char *win_name);
static void
JimNCurses_WindowId(Jim_Interp *interp, char *win_name, size_t len);

/**
 * package initializer, run by Jim at load time
 */
int
Jim_ncurses_extInit(Jim_Interp *interp) {
  if (Jim_PackageProvide(interp, "ncurses", "0.1", JIM_ERRMSG)) {
    return JIM_ERR;
  }

  Jim_CreateCommand(interp, "ncurses.init", JimNCursesCommand_init, NULL, NULL);
  Jim_CreateCommand(interp, "ncurses.isInitialized", JimNCursesCommand_isInitialized, NULL, NULL);
  Jim_CreateCommand(interp, "ncurses.end", JimNCursesCommand_end, NULL, NULL);
  Jim_CreateCommand(interp, "ncurses.refresh", JimNCursesCommand_refresh, NULL, NULL);
  Jim_CreateCommand(interp, "ncurses.getc", JimNCursesCommand_getc, NULL, NULL);

  return JIM_OK;
}

/**** implementations ****/
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

  if (win == NULL) {
    Jim_SetResultString(interp, "The window is null for some reason.", -1);
    return JIM_ERR;
  }

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

    if (sub == NULL) {
      Jim_SetResultString(interp,
        "failed to create window - possibly dimensions out of range?", -1
      );
      return JIM_ERR;
    }

    // make a window name token, and create the window proc
    char win_name[60];
    JimNCurses_WindowId(interp, win_name, 60);
    JimNCurses_CreateWindow(interp, sub, win_name);

    Jim_SetResultString(interp, win_name, -1);
    break;
  }

  return JIM_OK;
}

static void
JimNCurses_WindowId(Jim_Interp *interp, char *win_name, size_t len) {
  if (len == -1) len = strlen(win_name);

  snprintf(win_name, len, "ncurses.window<%ld>", Jim_GetId(interp));
}

/***
 * called by Jim when a window is destroyed
 */
static void
JimNCurses_DestroyWindow(Jim_Interp *interp, void *privData) {
  WINDOW *win = Jim_CmdPrivData(interp);
  delwin(win);
}

static void
JimNCurses_CreateWindow(Jim_Interp *interp, WINDOW *win, char *win_name) {
  Jim_CreateCommand(
    interp,
    win_name, // the command name
    JimNCursesCommand_handler, // the command itself
    win, // command's private data
    JimNCurses_DestroyWindow // destructor
  );
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

// ncurses.isInitialized
static int
JimNCursesCommand_isInitialized(Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  // we're in ncurses mode iff stdscr actually exists.
  Jim_SetResultBool(interp, stdscr != NULL);
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

  Jim_DeleteCommand(interp, "ncurses.stdscr");
  return JIM_OK;
}

// ncurses.getc
static int
JimNCursesCommand_getc(Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  char ch = getch();
  Jim_SetResultString(interp, &ch, 1);
  return JIM_OK;
}
