/* Test suite for argp.
   Copyright (C) 2006-2007, 2009-2021 Free Software Foundation, Inc.
   This file is part of the GNUlib Library.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

#include "argp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if HAVE_STRINGS_H
# include <strings.h>
#endif

#include "macros.h"

struct test_args
{
  int test;
  int verbose;
  char *file;
  int read;
  char *hidden;
  int opt;
  char *optional;
  int optional_set;
  int group_2_1_option;
  int group_1_1_option;
};

static struct argp_option group1_option[] = {
  { NULL, 0, NULL, 0, "Option Group 1", 0 },
  { "verbose", 'v', NULL, 0, "Simple option without arguments", 1 },
  { "file", 'f', "FILE", 0, "Option with a mandatory argument", 1 },
  { "input", 0, NULL, OPTION_ALIAS, NULL, 1 },
  { "read", 'r', NULL, OPTION_ALIAS, NULL, 1 },
  { "hidden", 'H', "FILE", OPTION_HIDDEN, "Hidden option", 1 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

static error_t
group1_parser (int key, char *arg, struct argp_state *state)
{
  struct test_args *args = state->input;

  switch (key)
    {
    case 'v':
      args->verbose++;
      break;

    case 'r':
      args->read = 1;
      FALLTHROUGH;
    case 'f':
      args->file = arg;
      break;

    case 'H':
      args->hidden = arg;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

struct argp group1_argp = {
  group1_option,
  group1_parser
};

struct argp_child group1_child = {
  &group1_argp,
  0,
  "",
  1
};


static struct argp_option group1_1_option[] = {
  { NULL, 0, NULL, 0, "Option Group 1.1", 0 },
  { "cantiga", 'C', NULL, 0, "create a cantiga" },
  { "sonet", 'S', NULL, 0, "create a sonet" },
  { NULL, 0, NULL, 0, NULL, 0 }
};

static error_t
group1_1_parser (int key, char *arg, struct argp_state *state)
{
  struct test_args *args = state->input;
  switch (key)
    {
    case 'C':
    case 'S':
      args->group_1_1_option = key;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

struct argp group1_1_argp = {
  group1_1_option,
  group1_1_parser
};

struct argp_child group1_1_child = {
  &group1_1_argp,
  0,
  "",
  2
};


static struct argp_option group2_option[] = {
  { NULL, 0, NULL, 0, "Option Group 2", 0 },
  { "option", 'O', NULL, 0, "An option", 1 },
  { "optional", 'o', "ARG", OPTION_ARG_OPTIONAL,
    "Option with an optional argument. ARG is one of the following:", 2 },
  { "one", 0, NULL, OPTION_DOC | OPTION_NO_TRANS, "one unit", 3 },
  { "two", 0, NULL, OPTION_DOC | OPTION_NO_TRANS, "two units", 3 },
  { "many", 0, NULL, OPTION_DOC | OPTION_NO_TRANS, "many units", 3 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

static error_t
group2_parser (int key, char *arg, struct argp_state *state)
{
  struct test_args *args = state->input;

  switch (key)
    {
    case 'O':
      args->opt = 1;
      break;

    case 'o':
      args->optional_set = 1;
      args->optional = arg;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

struct argp group2_argp = {
  group2_option,
  group2_parser
};

struct argp_child group2_child = {
  &group2_argp,
  0,
  "",
  2
};


static struct argp_option group2_1_option[] = {
  { NULL, 0, NULL, 0, "Option Group 2.1", 0 },
  { "poem", 'p', NULL, 0, "create a poem" },
  { "limerick", 'l', NULL, 0, "create a limerick" },
  { NULL, 0, NULL, 0, NULL, 0 }
};

static error_t
group2_1_parser (int key, char *arg, struct argp_state *state)
{
  struct test_args *args = state->input;
  switch (key)
    {
    case 'p':
    case 'e':
      args->group_2_1_option = key;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

struct argp group2_1_argp = {
  group2_1_option,
  group2_1_parser
};

struct argp_child group2_1_child = {
  &group2_1_argp,
  0,
  "",
  2
};


static struct argp_option main_options[] = {
  { NULL, 0, NULL, 0, "Main options", 0 },
  { "test", 't', NULL, 0, NULL, 1 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct test_args *args = state->input;
  int i;

  switch (key)
    {
    case ARGP_KEY_INIT:
      for (i = 0; state->root_argp->children[i].argp; i++)
        state->child_inputs[i] = args;
      break;

    case 't':
      args->test = 1;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

const char *argp_program_version = "test_argp (" PACKAGE_NAME ") " VERSION;
const char *argp_program_bug_address = "<" PACKAGE_BUGREPORT ">";
static char doc[] = "documentation string";

struct argp test_argp = {
  main_options,
  parse_opt,
  "ARGS...",
  doc,
  NULL,
  NULL,
  NULL
};

#define NARGS(a) (sizeof(a) / sizeof((a)[0]) - 1)
#define ARGV0 "test-argp"
#define init_args(a) memset (&(a), 0, sizeof (a));

#define INIT_TEST_COMMON(n)     \
 int argc = NARGS (argv);       \
 struct test_args test_args;    \
 init_args (test_args);         \
 test_number = n;

#define INIT_TEST1(n, arg1)            \
 char *argv[] = { ARGV0, arg1, NULL }; \
 INIT_TEST_COMMON (n)

#define INIT_TEST2(n, arg1, arg2)            \
 char *argv[] = { ARGV0, arg1, arg2, NULL }; \
 INIT_TEST_COMMON (n)

#define INIT_TEST3(n, arg1, arg2, arg3)            \
 char *argv[] = { ARGV0, arg1, arg2, arg3, NULL }; \
 INIT_TEST_COMMON (n)

int test_number;
unsigned failure_count = 0;

static void
fail (const char *msg)
{
  fprintf (stderr, "Test %d: %s\n", test_number, msg);
  failure_count++;
}

static void
test1 (struct argp *argp)
{
  INIT_TEST1 (1, "--test");
  if (argp_parse (argp, argc, argv, 0, NULL, &test_args))
    fail ("argp_parse failed");
  else if (test_args.test != 1)
    fail ("option not processed");
}

static void
test2 (struct argp *argp)
{
  INIT_TEST1 (2, "-t");
  if (argp_parse (argp, argc, argv, 0, NULL, &test_args))
    fail ("argp_parse failed");
  else if (test_args.test != 1)
    fail ("option not processed");
}

static void
test_file (struct argp *argp, int argc, char **argv, struct test_args *args)
{
  if (argp_parse (argp, argc, argv, 0, NULL, args))
    fail ("argp_parse failed");
  else if (!args->file)
    fail ("option not processed");
  else if (strcmp (args->file, "FILE"))
    fail ("option processed incorrectly");
}

static void
test3 (struct argp *argp)
{
  INIT_TEST1 (3, "--file=FILE");
  test_file (argp, argc, argv, &test_args);
}

static void
test4 (struct argp *argp)
{
  INIT_TEST2 (4, "--file", "FILE");
  test_file (argp, argc, argv, &test_args);
}

static void
test5 (struct argp *argp)
{
  INIT_TEST1 (5, "--input=FILE");
  test_file (argp, argc, argv, &test_args);
}

static void
test6 (struct argp *argp)
{
  INIT_TEST2 (6, "--input", "FILE");
  test_file (argp, argc, argv, &test_args);
}

static void
test_optional (struct argp *argp, int argc, char **argv,
               struct test_args *args, const char *val, const char *a)
{
  int index;
  if (argp_parse (argp, argc, argv, 0, &index, args))
    fail ("argp_parse failed");
  else if (!args->optional_set)
    fail ("option not processed");

  if (!val)
    {
      if (args->optional)
        fail ("option processed incorrectly");
    }
  else if (strcmp (args->optional, val))
    fail ("option processed incorrectly");

  if (a)
    {
      if (index == argc)
        fail ("expected command line argument not found");
      else if (strcmp (argv[index], a))
        fail ("expected command line argument does not match");
    }
}

static void
test7 (struct argp *argp)
{
  INIT_TEST1 (7, "-oARG");
  test_optional (argp, argc, argv, &test_args, "ARG", NULL);
}

static void
test8 (struct argp *argp)
{
  INIT_TEST2 (8, "-o", "ARG");
  test_optional (argp, argc, argv, &test_args, NULL, "ARG");
}

static void
test9 (struct argp *argp)
{
  INIT_TEST1 (9, "--optional=ARG");
  test_optional (argp, argc, argv, &test_args, "ARG", NULL);
}

static void
test10 (struct argp *argp)
{
  INIT_TEST2 (10, "--optional", "ARG");
  test_optional (argp, argc, argv, &test_args, NULL, "ARG");
}

static void
test11 (struct argp *argp)
{
  INIT_TEST1 (11, "--optiona=ARG");
  test_optional (argp, argc, argv, &test_args, "ARG", NULL);
}

static void
test12 (struct argp *argp)
{
  INIT_TEST3 (12, "--option", "--optional=OPT", "FILE");
  test_optional (argp, argc, argv, &test_args, "OPT", "FILE");
}

static void
test13 (struct argp *argp)
{
  INIT_TEST1 (1, "--cantiga");
  if (argp_parse (argp, argc, argv, 0, NULL, &test_args))
    fail ("argp_parse failed");
  else if (test_args.group_1_1_option != 'C')
    fail ("option not processed");
}

static void
test14 (struct argp *argp)
{
  INIT_TEST1 (1, "--limerick");
  if (argp_parse (argp, argc, argv, 0, NULL, &test_args))
    fail ("argp_parse failed");
  else if (test_args.group_2_1_option != 'l')
    fail ("option not processed");
}

static void
test15 (struct argp *argp)
{
  INIT_TEST2 (1, "-r", "FILE");
  test_file (argp, argc, argv, &test_args);
  if (!test_args.read)
    fail ("short alias not recognized properly");
}


typedef void (*test_fp) (struct argp *argp);

static test_fp test_fun[] = {
  test1,  test2,  test3,  test4,
  test5,  test6,  test7,  test8,
  test9,  test10, test11, test12,
  test13, test14, test15,
  NULL
};

int
main (int argc, char **argv)
{
  struct argp_child argp_children[3], group1_children[2], group2_children[2];
  test_fp *fun;

  group1_children[0] = group1_1_child;
  group1_children[1].argp = NULL;
  group1_argp.children = group1_children;

  group2_children[0] = group2_1_child;
  group2_children[1].argp = NULL;
  group2_argp.children = group2_children;

  argp_children[0] = group1_child;
  argp_children[1] = group2_child;
  argp_children[2].argp = NULL;
  test_argp.children = argp_children;

  if (argc > 0)
    {
      struct test_args test_args;
      init_args (test_args);
      return argp_parse (&test_argp, argc, argv, 0, NULL, &test_args);
    }

  for (fun = test_fun; *fun; fun++)
    (*fun) (&test_argp);

  if (failure_count)
    return 1;

  return 0;
}
