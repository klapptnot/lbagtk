#include "arg_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_help (const char *program_name) {
  printf ("%s%s%s: Low battery alert\n", COLOR_BOLD_BLUE, program_name, COLOR_RESET);
  printf ("\n");
  printf ("%sUsage%s:\n", COLOR_BOLD_BLUE, COLOR_RESET);
  printf ("  %s%s%s [OPTIONS]\n", COLOR_CYAN, program_name, COLOR_RESET);
  printf ("  %s%s%s help\n", COLOR_CYAN, program_name, COLOR_RESET);
  printf ("\n");
  printf ("%sOptions%s:\n", COLOR_BOLD_BLUE, COLOR_RESET);
  printf ("  -D, --daemon            Run as a background, detached process\n");
  printf (
      "  -s, --styles %s<file>%s     Path to styles file (default: %s)\n",
      COLOR_WHITE,
      COLOR_RESET,
      DEFAULT_STYLES_PATH
  );
  printf (
      "  -l, --low %s<level>%s       Low battery level (default: 20)\n",
      COLOR_WHITE,
      COLOR_RESET
  );
  printf (
      "  -r, --risky %s<level>%s     Risky battery level, activating secondary button "
      "(default: 10)\n",
      COLOR_WHITE,
      COLOR_RESET
  );
  printf (
      "  -b, --btn %s<text>%s        Secondary button text (default: '%s')\n",
      COLOR_WHITE,
      COLOR_RESET,
      DEFAULT_BUTTON_TEXT
  );
  printf (
      "  -B, --btn-cmd %s<cmd>%s     Secondary button command (default: '%s')\n",
      COLOR_WHITE,
      COLOR_RESET,
      DEFAULT_BUTTON_CMDL
  );
  printf ("  -h, --help              Show this help message and exit\n");
  printf ("\n");
  printf ("%sExamples:%s\n", COLOR_BOLD_BLUE, COLOR_RESET);
  printf ("  %s -l 15 -r 5\n", program_name);
  printf ("  %s --daemon --styles /home/ari/.config/lowbat.css\n", program_name);
  printf ("  %s -D -b \"Suspend\" -B \"systemctl suspend\"\n", program_name);
  printf ("  %s --low 25 --risky 8 --daemon\n", program_name);
}

AppOpts *create_default_opts (void) {
  AppOpts *opts = malloc (sizeof (AppOpts));
  if (!opts) {
    fprintf (stderr, "Memory allocation failed\n");
    exit (EXIT_FAILURE);
  }

  opts->css_file = strdup (DEFAULT_STYLES_PATH);
  opts->btn_str = strdup (DEFAULT_BUTTON_TEXT);
  opts->btn_cmd = strdup (DEFAULT_BUTTON_CMDL);
  opts->low_lvl = 20;
  opts->risk_lvl = 10;
  opts->daemon = false;

  if (!opts->css_file || !opts->btn_str || !opts->btn_cmd) {
    fprintf (stderr, "String duplication failed\n");
    exit (EXIT_FAILURE);
  }

  return opts;
}

void free_opts (AppOpts *opts) {
  if (opts) {
    free (opts->css_file);
    free (opts->btn_str);
    free (opts->btn_cmd);
    free (opts);
  }
}

char get_short_opt (const char *arg) {
  const char *opt = arg;
  if (*opt == '-') opt++;
  if (*opt != '-') return *opt;
  opt++;

  if (strlen (opt) > 1) {
    if (strcmp (opt, "help") == 0) return 'h';
    if (strcmp (opt, "daemon") == 0) return 'D';
    if (strcmp (opt, "styles") == 0) return 's';
    if (strcmp (opt, "low") == 0) return 'l';
    if (strcmp (opt, "risky") == 0) return 'r';
    if (strcmp (opt, "btn") == 0) return 'b';
    if (strcmp (opt, "btn-cmd") == 0) return 'B';
    return '?';
  } else
    return '?';

  return *opt;
}

AppOpts *parse_arguments (int argc, char **argv) {
  AppOpts *opts = create_default_opts ();

  char *program_name;
  {
    const char *base = *argv;
    for (const char *p = *argv; *p; p++) {
      if (*p == '/') base = p + 1;
    }

    program_name = *base ? (char *)base : *argv;
  }

  argc--;
  argv++;

  while (argc > 0) {
    char *arg = popf (argc, argv);

    if (strncmp (arg, "--miku", 7) == 0) {
      printf (
          "\x1b[38;5;86m"
          "🌸 初音ミクはみんなのことが大好きだよ！🌸\n"
          " Hatsune Miku loves everyone very much!\n"
          "\x1b[0m"
      );
      continue;
    }

    if (*arg != '-') {
      fprintf (stderr, "Error: Unknown option '%s'\n", arg);
      fprintf (stderr, "Try '%s --help' for more information.\n", program_name);
      free_opts (opts);
      return NULL;
    }

    char opt = get_short_opt (arg);

    switch (opt) {
      case 'h':
        print_help (program_name);
        free_opts (opts);
        exit (EXIT_SUCCESS);

      case 'D':
        opts->daemon = true;
        break;

      case 's':
        if (argc == 0) {
          fprintf (stderr, "Error: %s requires an argument\n", arg);
          free_opts (opts);
          return NULL;
        }
        free (opts->css_file);
        opts->css_file = strdup (popf (argc, argv));
        if (!opts->css_file) {
          fprintf (stderr, "String duplication failed\n");
          free_opts (opts);
          return NULL;
        }
        break;

      case 'l': {
        if (argc == 0) {
          fprintf (stderr, "Error: %s requires an argument\n", arg);
          free_opts (opts);
          return NULL;
        }
        char *level_str = popf (argc, argv);
        char *endptr;
        long level = strtol (level_str, &endptr, 10);
        if (*endptr != '\0' || level < 0 || level > 100) {
          fprintf (stderr, "Error: Invalid level '%s' (must be 0-100)\n", level_str);
          free_opts (opts);
          return NULL;
        }
        opts->low_lvl = (int)level;
        break;
      }

      case 'r': {
        if (argc == 0) {
          fprintf (stderr, "Error: %s requires an argument\n", arg);
          free_opts (opts);
          return NULL;
        }
        char *level_str = popf (argc, argv);
        char *endptr;
        long level = strtol (level_str, &endptr, 10);
        if (*endptr != '\0' || level < 0 || level > 100) {
          fprintf (stderr, "Error: Invalid level '%s' (must be 0-100)\n", level_str);
          free_opts (opts);
          return NULL;
        }
        opts->risk_lvl = (int)level;
        break;
      }

      case 'b':
        if (argc == 0) {
          fprintf (stderr, "Error: %s requires an argument\n", arg);
          free_opts (opts);
          return NULL;
        }
        free (opts->btn_str);
        opts->btn_str = strdup (popf (argc, argv));
        if (!opts->btn_str) {
          fprintf (stderr, "String duplication failed\n");
          free_opts (opts);
          return NULL;
        }
        break;

      case 'B':
        if (argc == 0) {
          fprintf (stderr, "Error: %s requires an argument\n", arg);
          free_opts (opts);
          return NULL;
        }
        free (opts->btn_cmd);
        opts->btn_cmd = strdup (popf (argc, argv));
        if (!opts->btn_cmd) {
          fprintf (stderr, "String duplication failed\n");
          free_opts (opts);
          return NULL;
        }
        break;

      default:
        fprintf (stderr, "Error: Unknown option '%s'\n", arg);
        fprintf (stderr, "Try '%s --help' for more information.\n", program_name);
        free_opts (opts);
        return NULL;
    }
  }

  // Validation: risky level should be <= low level
  // Not wrong, but makes the secondary button active + notify once
  if (opts->risk_lvl > opts->low_lvl) {
    fprintf (
        stderr,
        "\x1b[38;5;11mWarning: Risky level (%d) is higher than low level (%d)\x1b[0m\n",
        opts->risk_lvl,
        opts->low_lvl
    );
  }

  return opts;
}
