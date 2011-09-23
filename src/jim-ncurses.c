#include <assert.h>
#include <string.h>
#include <ncurses.h>
#include <jim.h>

/************ prototypes ************/

// window command handler
static int
JimNCursesCommand_handler(Jim_Interp *interp, int argc, Jim_Obj *const *argv);
static int
JimNCursesCallCustomMethod(Jim_Interp *interp, int argc, Jim_Obj *const *argv);

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

/*****
 * This is the command that's called on a window object.
 *
 * % set win [stdscr window 10 10 0 0]
 * % $win <method> <args>
 *
 * if it's not in the command table here, it's delegated to
 *
 * ncurses.window::<method>
 *
 * and the window is sent in as the first argument.
 */
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
    "mvaddstr",
    "box",
    "window",
    "getmaxyx",
    NULL
  };

  enum {
    OPT_REFRESH,
    OPT_MVADDSTR,
    OPT_BOX,
    OPT_WINDOW,
    OPT_GETMAXYX
  };

  // figure out which method was called, and call the custom method if it's
  // not defined here.
  if (Jim_GetEnum(interp, argv[1], options, &option, "NCurses method", 0) != JIM_OK) {
    return JimNCursesCallCustomMethod(interp, argc, argv);
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
  // refreshes the window
  case OPT_REFRESH:
    wrefresh(win);
    break;

  // outputs text to the window (without a newline!)
  case OPT_MVADDSTR:
    if (argc != 3) {
      Jim_WrongNumArgs(interp, argc, argv, "mvaddstr row col string");
    }

    long row = 0, col = 0;
    if (Jim_GetLong(interp, argv[1], &row) != JIM_OK) {
      Jim_SetResultFormatted(interp, "expected number for row, got \"%#s\"", argv[1]);
      return JIM_ERR;
    }
    if (Jim_GetLong(interp, argv[2], &col) != JIM_OK) {
      Jim_SetResultFormatted(interp, "expected number for col, got \"%#s\"", argv[2]);
      return JIM_ERR;
    }

    mvwaddstr(win, row, col, Jim_String(argv[3]));
    wrefresh(win);
    refresh();
    Jim_SetResult(interp, argv[1]);
    break;

  // draws a box around the window
  case OPT_BOX:
    wborder(win, '|', '|', '-', '-', '+', '+', '+', '+');
    break;

  // creates a new subwindow
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

  case OPT_GETMAXYX:
    if (argc > 1) {
      Jim_WrongNumArgs(interp, 1, argv, "getmaxyx takes no arguments");
    }

    int x;
    int y;
    Jim_Obj *retList;

    getmaxyx(win, x, y);
    retList = Jim_NewListObj(interp, NULL, 0);
    Jim_ListAppendElement(interp, retList, Jim_NewIntObj(interp, x));
    Jim_ListAppendElement(interp, retList, Jim_NewIntObj(interp, y));

    Jim_SetResult(interp, retList);
    break;
  }

  // since we caught the undefined method case above, we know we're ok here
  return JIM_OK;
}

/******
 * a helper to _handler which allows custom methods to be called.  It transforms
 *
 *    $window method arg1 arg2
 *
 * into
 *
 *    ncurses.window::method $window arg1 arg2
 *
 */
static int
JimNCursesCallCustomMethod(Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  assert(argc > 0);

  // construct the method name, which is "ncurses.window::$method"
  Jim_Obj *cmdName = Jim_NewStringObj(interp, "ncurses.window::", -1);
  Jim_AppendObj(interp, cmdName, argv[1]);

  // build the new command vector
  //
  // ncurses.window::method $window    arg1 arg2 ...
  // $cmdName $argv(0) $argv(2) $argv(3) ...
  Jim_Obj **newargv = Jim_Alloc(argc * sizeof(Jim_Obj *));
  newargv[0] = cmdName;
  newargv[1] = argv[0];
  memcpy(newargv + 2, argv + 2, (argc - 2) * sizeof(Jim_Obj *));

  // call the command with the appropriate args list
  int retCode = Jim_EvalObjVector(interp, argc, newargv);

  Jim_Free(newargv);

  return retCode;
}

/****
 * prints a unique window name into the given char *.
 */
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

/***
 * create a window command, given a window and a name
 */
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
// begins ncurses mode, and creates stdscr
static int
JimNCursesCommand_init(Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  initscr();
  cbreak();

  JimNCurses_CreateWindow(interp, stdscr, "ncurses.stdscr");

  return JIM_OK;
}

// ncurses.isInitialized
// test if the terminal is in ncurses mode
static int
JimNCursesCommand_isInitialized(Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  // we're in ncurses mode iff stdscr actually exists.
  Jim_SetResultBool(interp, stdscr != NULL);
  return JIM_OK;
}

// ncurses.end
// ends ncurses mode.  Be sure to call this before the program ends!
// Much better is to use `ncurses.do {script}`, which handles all the
// setup and teardown transparently.
static int
JimNCursesCommand_end(Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  endwin();

  Jim_DeleteCommand(interp, "ncurses.stdscr");
  return JIM_OK;
}

// ncurses.refresh
// force a refresh
static int
JimNCursesCommand_refresh(Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  refresh();
  return JIM_OK;
}

// ncurses.getc
// gets a character
static int
JimNCursesCommand_getc(Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  char ch = getch();
  Jim_SetResultString(interp, &ch, 1);
  return JIM_OK;
}
