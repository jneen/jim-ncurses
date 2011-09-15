#include <ncursesw/curses.h>
#include <jim.h>

static int
JimNCursesCommand_handler(Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  // return an error if no method is specified
  if (argc < 2) {
    Jim_WrongNumArgs(interp, 1, argv, "method ?args ...?");
  }

  // the ghetto-fabulous command table
  int option;
  static const char * const options[] = {
    "puts",
    NULL
  };

  enum {
    OPT_PUTS
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
    case OPT_PUTS:
      if (argc < 2) {
        Jim_WrongNumArgs(interp, argc, argv, "puts string");
        return JIM_ERR;
      }

      waddstr(win, Jim_String(argv[1]));
      Jim_SetResult(interp, argv[1]);
      return JIM_OK;
  }

	return JIM_ERR;
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
    snprintf(win_name, 1+60*sizeof(win_name), "ncurses.window<%ld>", Jim_GetId(interp));
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

static int
JimNCursesCommand_window(Jim_Interp *interp, int argc, Jim_Obj *const *argv) {
  if (argc < 5) {
    Jim_WrongNumArgs(interp, argc, argv, "height width row column");
    return JIM_ERR;
  }

  WINDOW *win = newwin(10, 10, 0, 0);

  Jim_SetResult(interp, JimNCurses_CreateWindow(interp, win, NULL));
  return JIM_OK;
}

int
Jim_ncursesInit(Jim_Interp *interp) {
  if (Jim_PackageProvide(interp, "ncurses", "0.1", JIM_ERRMSG)) {
    return JIM_ERR;
  }

  JimNCurses_CreateWindow(interp, stdscr, "ncurses.stdscr");
  Jim_CreateCommand(interp, "ncurses.window", JimNCursesCommand_window, NULL, NULL);

	return JIM_OK;
}
