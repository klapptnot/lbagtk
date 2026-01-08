#ifndef ARG_PARSER_H
#define ARG_PARSER_H

#define DEFAULT_STYLES_PATH "~/.config/gall/lbagtk.css"
#define DEFAULT_BUTTON_TEXT "Suspend"
#define DEFAULT_BUTTON_CMDL "systemctl suspend"

#define COLOR_RESET     "\x1b[0m"
#define COLOR_BOLD_BLUE "\x1b[1;34m"
#define COLOR_CYAN      "\x1b[38;5;14m"
#define COLOR_WHITE     "\x1b[38;5;15m"

//~ Print formatted error message to stderr with red coloring
#define eprintf(fmt, ...) \
  fprintf (stderr, "\x1b[38;5;9mError: " fmt "\x1b[0m" __VA_OPT__ (, ) __VA_ARGS__)

// Pop an element from the beginning of an array
#define popf(c, v) \
  (c > 0           \
     ? (--c, *v++) \
     : (eprintf ("Trying to access a non-existent value\n"), exit (EXIT_FAILURE), (void*)0))

typedef struct {
  char* css_file;  // Path to CSS styles file
  char* btn_str;   // Secondary button text
  char* btn_cmd;   // Secondary button command
  int low_lvl;     // Low battery level threshold
  int risk_lvl;    // Critical battery level threshold
  bool daemon;     // Run as daemon flag
} AppOpts;

void print_help (const char* program_name);
AppOpts* create_default_opts (void);
void free_opts (AppOpts* opts);
char get_short_opt (const char* arg);
AppOpts* parse_arguments (int argc, char** argv);

#endif  // ARG_PARSER_H
