#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <ncurses.h>
#include <jim.h>

#define JIM_METHOD(name) static int name(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
#define JIM_NCURSES_METHOD(name) static int name(Jim_Interp *interp, WINDOW *win, int argc, Jim_Obj *const *argv)

/************ prototypes ************/

// window command handler
static int
JimNCursesCommand_handler(Jim_Interp *interp, int argc, Jim_Obj *const *argv);
static int
JimNCursesCallCustomMethod(Jim_Interp *interp, int argc, Jim_Obj *const *argv);

// window methods
JIM_NCURSES_METHOD(JimNCursesMethod_box);
JIM_NCURSES_METHOD(JimNCursesMethod_getc);
JIM_NCURSES_METHOD(JimNCursesMethod_getmaxyx);
JIM_NCURSES_METHOD(JimNCursesMethod_move);
JIM_NCURSES_METHOD(JimNCursesMethod_mvaddstr);
JIM_NCURSES_METHOD(JimNCursesMethod_refresh);
JIM_NCURSES_METHOD(JimNCursesMethod_window);

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
    "box",
    "getc",
    "getmaxyx",
    "move",
    "mvaddstr",
    "refresh",
    "window",
    NULL
  };

  enum {
    OPT_BOX,
    OPT_GETC,
    OPT_GETMAXYX,
    OPT_MOVE,
    OPT_MVADDSTR,
    OPT_REFRESH,
    OPT_WINDOW
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
  case OPT_BOX:      return JimNCursesMethod_box(interp, win, argc, argv);
  case OPT_GETC:     return JimNCursesMethod_getc(interp, win, argc, argv);
  case OPT_GETMAXYX: return JimNCursesMethod_getmaxyx(interp, win, argc, argv);
  case OPT_MOVE:     return JimNCursesMethod_move(interp, win, argc, argv);
  case OPT_MVADDSTR: return JimNCursesMethod_mvaddstr(interp, win, argc, argv);
  case OPT_REFRESH:  return JimNCursesMethod_refresh(interp, win, argc, argv);
  case OPT_WINDOW:   return JimNCursesMethod_window(interp, win, argc, argv);
  }

  // since we caught the undefined method case above, we shouldn't get here
  return JIM_ERR;
}

JIM_NCURSES_METHOD(JimNCursesMethod_box) {
  wborder(win, '|', '|', '-', '-', '+', '+', '+', '+');
  return JIM_OK;
}

JIM_NCURSES_METHOD(JimNCursesMethod_getc) {
  if (argc > 1) {
    Jim_WrongNumArgs(interp, 1, argv, "getc takes no arguments");
  }

  int code = wgetch(win);

  // if it's a printable character, just return it
  if (isprint(code) && !isspace(code)) {
    char ch = (char) code;
    Jim_SetResultString(interp, &ch, 1);
  }
  else {
    switch(code) {
    // arrow keys
    case KEY_UP:    Jim_SetResultString(interp, "<Up>",   -1); break;
    case KEY_DOWN:  Jim_SetResultString(interp, "<Down>", -1); break;
    case KEY_LEFT:  Jim_SetResultString(interp, "<Left>", -1); break;
    case KEY_RIGHT: Jim_SetResultString(interp, "<Right>",-1); break;

    case KEY_BACKSPACE: Jim_SetResultString(interp, "<Backspace>", -1); break;

    // enter key, also '\n'
    case KEY_ENTER:
    case 10: Jim_SetResultString(interp, "<Enter>", -1); break;

    default:
      Jim_SetResultFormatted(interp, "<0x%x>", code);
    }
  }

  return JIM_OK;
}

JIM_NCURSES_METHOD(JimNCursesMethod_getmaxyx) {
  if (argc > 1) {
    Jim_WrongNumArgs(interp, 1, argv, "getmaxyx takes no arguments");
    return JIM_ERR;
  }

  int x;
  int y;
  Jim_Obj *retList;

  getmaxyx(win, x, y);
  retList = Jim_NewListObj(interp, NULL, 0);
  Jim_ListAppendElement(interp, retList, Jim_NewIntObj(interp, x));
  Jim_ListAppendElement(interp, retList, Jim_NewIntObj(interp, y));

  Jim_SetResult(interp, retList);
  return JIM_OK;
}

JIM_NCURSES_METHOD(JimNCursesMethod_move) {
  if (argc < 3) {
    Jim_WrongNumArgs(interp, 3, argv, "move row col");
    return JIM_ERR;
  }

  long row;
  long col;
  Jim_GetLong(interp, argv[1], &row);
  Jim_GetLong(interp, argv[2], &col);

  wmove(win, row, col);
  wrefresh(win);

  return JIM_OK;
}

JIM_NCURSES_METHOD(JimNCursesMethod_mvaddstr) {
  if (argc != 4) {
    Jim_WrongNumArgs(interp, argc, argv, "mvaddstr row col string");
    return JIM_ERR;
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
  Jim_SetResult(interp, argv[1]);

  return JIM_OK;
}

JIM_NCURSES_METHOD(JimNCursesMethod_refresh) {
  wrefresh(win);
  return JIM_OK;
}

JIM_NCURSES_METHOD(JimNCursesMethod_window) {
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
  keypad(stdscr, TRUE);
  noecho();

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
  Jim_SetResultString(interp, "ncurses.getc is deprecated, use stdscr getc instead", -1);
  return JIM_ERR;
}
