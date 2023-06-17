/*
 * Converts binary data into a C source file.  The C source file
 * defines a string with the file name of the source of data, and an
 * unsigned character array containing the binary data.
 *
 * A useful GNUMakefile rule follows.
 *
 * %.h:    %.lua
 *         luac -o $*.luo $*.lua
 *         bin2c -o $@ -n $*.lua $*.luo
 *         rm $*.luo
 *
 * This version was modified from the original for use in mame2003-plus.
 *
 * Original version credited by John D. Ramsdell - Copyright (C) 2006 The MITRE Corporation.
 */

#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

#define COLUMNS 18

#ifdef PACKAGE_NAME
static const char package[] = PACKAGE_NAME;
#else
static const char *package = NULL;
#endif

#ifdef VERSION
static const char version[] = VERSION;
#else
static const char version[] = "version unknown";
#endif

static void
print_version(const char *program)
{
  if (package != NULL)
    program = package;
  fprintf(stderr, "%s %s\n", program, version);
}

static void
usage(const char *prog)
{
  fprintf(stderr,
	  "Usage: %s [options] file\n"
	  "Options:\n"
	  "  -n name -- generated C identifier source (default is file)\n"
	  "  -o file -- output to file (default is standard output)\n"
	  "  -v      -- print version information\n"
	  "  -h      -- print this message\n",
	  prog);
}

static void
emit_name(const char *name)
{
  int ch = *name;
  if (!ch) {
    putchar('_');
    return;
  }
  if (isalpha(ch))		/* Print underscore */
    putchar(ch);		/* when first char is */
  else				/* is not a letter. */
    putchar('_');
  for (;;) {
    ch = *++name;
    if (!ch || ch == 0x2e)	/* return if no value or reached file extension */
      return;
    if (isalnum(ch))		/* Print underscore when */
      putchar(ch);		/* part of identifier is */
    else			/* not a letter or a digit. */
      putchar('_');
  }
}

static int
emit(const char *name)
{
  int file_length = 0;
  int col = COLUMNS;

  printf("const struct bin2cFILE ");
  emit_name(name);
  printf("_bootstrap = {");
  printf("\n    ?,\n  {");
  for (;;) {
    int ch = getchar();
    if (ch == EOF) {
      printf("\n  }\n};");
      emit_name(name);
      printf("_length = %i", file_length);      
      return 0;
    }
    if (col >= COLUMNS) {
      printf("\n  ");
      col = 0;
    }
    printf("%3d,", ch);
    file_length++;
    col++;
  }
}

int
main(int argc, char *argv[])
{
  extern char *optarg;
  extern int optind;

  char *input = NULL;
  char *output = NULL;
  char *name = NULL;

  for (;;) {
    int c = getopt(argc, argv, "n:o:vh");
    if (c == -1)
      break;
    switch (c) {
    case 'n':
      name = optarg;
      break;
    case 'o':
      output = optarg;
      break;
    case 'v':
      print_version(argv[0]);
      return 0;
    case 'h':
      usage(argv[0]);
      return 0;
    default:
      usage(argv[0]);
      return 1;
    }
  }

  if (argc != optind + 1) {
    fprintf(stderr, "Bad arg count\n");
    usage(argv[0]);
    return 1;
  }

  input = argv[optind];
  if (!freopen(input, "rb", stdin)) {
    perror(input);
    return 1;
  }

  if (output && !freopen(output, "w", stdout)) {
    perror(output);
    return 1;
  }

  return emit(name ? name : input);
}
