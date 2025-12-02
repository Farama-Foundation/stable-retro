/* Test of system-quote module.
   Copyright (C) 2012-2021 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2012.  */

#include <config.h>

/* Specification.  */
#include "system-quote.h"

#if defined _WIN32 && !defined __CYGWIN__
# define WINDOWS_NATIVE
#endif

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef WINDOWS_NATIVE
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif

#include "macros.h"

#define EXPECTED_DATA_FILE "t-sq-data.tmp"

static int failed;

static void
check_one (enum system_command_interpreter interpreter, const char *prog,
           const char *input)
{
  char buf[1000];
  size_t output_len;
  char *output;
  char *bufend;

  output_len = system_quote_length (interpreter, input);

  output = system_quote (interpreter, input);
  ASSERT (strlen (output) == output_len);

  ASSERT (output_len <= sizeof (buf) - 2);
  memset (buf, '\0', output_len + 1);
  buf[output_len + 1] = '%';
  bufend = system_quote_copy (buf, interpreter, input);
  ASSERT (bufend == buf + output_len);
  ASSERT (memcmp (buf, output, output_len + 1) == 0);
  ASSERT (buf[output_len + 1] == '%');

  /* Store INPUT in EXPECTED_DATA_FILE, for verification by the child
     process.  */
  {
    FILE *fp = fopen (EXPECTED_DATA_FILE, "wb");
    if (fp == NULL)
      exit (3);
    if (fwrite (input, 1, strlen (input), fp) != strlen (input))
      exit (4);
    if (fclose (fp))
      exit (5);
  }

  /* Invoke the child process through system() and popen().  */
  {
    char command[1000];

    sprintf (command, "%s %s", prog, output);

    switch (interpreter)
      {
      case SCI_SYSTEM:
#ifdef WINDOWS_NATIVE
      case SCI_WINDOWS_CMD:
#endif
        {
          int exitcode = system (command);
          if (exitcode != 0)
            {
              fprintf (stderr, "for input = |%s|: system() command failed with status %d: %s\n",
                       input, exitcode, command);
              failed = 1;
            }
        }
        {
          FILE *fp = popen (command, "r");
          int exitcode = pclose (fp);
          if (exitcode != 0)
            {
              fprintf (stderr, "for input = |%s|: popen() command failed with status %d: %s\n",
                       input, exitcode, command);
              failed = 1;
            }
        }
        break;
#ifdef WINDOWS_NATIVE
      case SCI_WINDOWS_CREATEPROCESS:
        {
          PROCESS_INFORMATION pinfo;
          STARTUPINFO sinfo;
          sinfo.cb = sizeof (STARTUPINFO);
          sinfo.lpReserved = NULL;
          sinfo.lpDesktop = NULL;
          sinfo.lpTitle = NULL;
          sinfo.cbReserved2 = 0;
          sinfo.lpReserved2 = NULL;
          sinfo.dwFlags = STARTF_USESTDHANDLES;
          sinfo.hStdInput = GetStdHandle (STD_INPUT_HANDLE);
          sinfo.hStdOutput = GetStdHandle (STD_OUTPUT_HANDLE);
          sinfo.hStdError = GetStdHandle (STD_ERROR_HANDLE);

          if (CreateProcess (NULL, command, NULL, NULL, TRUE, 0, NULL, NULL,
                             &sinfo, &pinfo))
            {
              DWORD exitcode;
              CloseHandle (pinfo.hThread);
              if (WaitForSingleObject (pinfo.hProcess, INFINITE) == WAIT_OBJECT_0)
                {
                  if (GetExitCodeProcess (pinfo.hProcess, &exitcode))
                    {
                      if (exitcode != 0)
                        {
                          fprintf (stderr, "for input = |%s|: CreateProcess() command failed with status %d: %s\n",
                                   input, exitcode, command);
                          failed = 1;
                        }
                    }
                  else
                    {
                      fprintf (stderr, "for input = |%s|: GetExitCodeProcess failed, GetLastError() = %u\n",
                               input, GetLastError ());
                      failed = 1;
                    }
                }
              else
                {
                  fprintf (stderr, "for input = |%s|: WaitForSingleObject failed\n",
                           input);
                  failed = 1;
                }
              CloseHandle (pinfo.hProcess);
            }
          else
            {
              fprintf (stderr, "for input = |%s|: CreateProcess failed, GetLastError() = %u\n",
                       input, GetLastError ());
              failed = 1;
            }
        }
        break;
#endif
      default:
        break;
      }
  }

  free (output);
}

static void
check_all (enum system_command_interpreter interpreter,
           bool windows_cmd_limitations,
           const char *prog)
{
  /* Check the system_quote_length, system_quote_copy, system_quote
     functions.  */
  {
    int c;

    /* Empty argument.  */
    check_one (interpreter, prog, "");

    /* Identifier or number.  */
    check_one (interpreter, prog, "foo");
    check_one (interpreter, prog, "phr0ck");

    /* Whitespace would be interpreted as argument separator by the shell.  */
    check_one (interpreter, prog, "foo\tbar");
    if (!windows_cmd_limitations)
      {
        check_one (interpreter, prog, "foo\nbar");
        check_one (interpreter, prog, "foo\rbar");
      }
    check_one (interpreter, prog, "foo bar");

    /* '!' at the beginning of argv[0] would introduce a negated command.  */
    check_one (interpreter, prog, "!foo");

    /* '"' would be interpreted as the start of a string.  */
    check_one (interpreter, prog, "\"foo\"bar");

    /* '#' at the beginning of an argument would be interpreted as the start
       of a comment.  */
    check_one (interpreter, prog, "#foo");

    /* '$' at the beginning of an argument would be interpreted as a variable
       reference.  */
    check_one (interpreter, prog, "$foo");

    /* '&' at the beginning of an argument would be interpreted as a background
       task indicator.  */
    check_one (interpreter, prog, "&");

    /* "'" would be interpreted as the start of a string.  */
    check_one (interpreter, prog, "'foo'bar");

    /* '(' at the beginning of argv[0] would introduce a subshell command.  */
    check_one (interpreter, prog, "(");

    /* ')' at the beginning of an argument would be interpreted as the end of
       the command.  */
    check_one (interpreter, prog, ")");

    /* '*' would be interpreted as a wildcard character.  */
    check_one (interpreter, prog, "*");
    check_one (interpreter, prog, "*foo");

    /* ';' at the beginning of an argument would be interpreted as an empty
       statement in argv[0] and as the end of the command otherwise.  */
    check_one (interpreter, prog, ";");
    check_one (interpreter, prog, "foo;");

    /* '<' would be interpreted as a redirection of stdin.  */
    check_one (interpreter, prog, "<");

    /* '=' inside argv[0] would be interpreted as an environment variable
       assignment.  */
    check_one (interpreter, prog, "foo=bar");

    /* '>' would be interpreted as a redirection of stdout.  */
    check_one (interpreter, prog, ">");

    /* '?' would be interpreted as a wildcard character.  */
    check_one (interpreter, prog, "?");
    check_one (interpreter, prog, "??");
    check_one (interpreter, prog, "???");
    check_one (interpreter, prog, "????");
    check_one (interpreter, prog, "?????");
    check_one (interpreter, prog, "??????");
    check_one (interpreter, prog, "???????");
    check_one (interpreter, prog, "????????");
    check_one (interpreter, prog, "?????????");
    check_one (interpreter, prog, "??????????");
    check_one (interpreter, prog, "foo?bar");

    /* '^' would be interpreted in old /bin/sh, e.g. SunOS 4.1.4.  */
    check_one (interpreter, prog, "^");

    /* "[...]" would be interpreted as a wildcard pattern.  */
    check_one (interpreter, prog, "[");
    check_one (interpreter, prog, "]");

    /* '\' would be interpreted as an escape character.  */
    check_one (interpreter, prog, "\\foo");

    /* '`' would be interpreted as the start of a command substitution.  */
    check_one (interpreter, prog, "`foo");

    /* '{' at the beginning of argv[0] would introduce a complex command.  */
    check_one (interpreter, prog, "{");

    /* '|' at the beginning of an argument would be interpreted as a pipe
       between commands.  */
    check_one (interpreter, prog, "|");

    /* '}' at the beginning of an argument would be interpreted as the end of
       the command.  */
    check_one (interpreter, prog, "}");

    /* '~' at the beginning of an argument would be interpreted as a reference
       to a user's home directory.  */
    check_one (interpreter, prog, "~");
    check_one (interpreter, prog, "~foo");

    /* A string that contains both ' and ".  */
    check_one (interpreter, prog, "foo'bar\"baz");

    /* '%' is used for environment variable references in Windows cmd.exe.  */
    check_one (interpreter, prog, "%");
    check_one (interpreter, prog, "%%");
    check_one (interpreter, prog, "%foo%");
    check_one (interpreter, prog, "%PATH%");

    /* All other characters don't need quoting.  */
    for (c = 1; c <= UCHAR_MAX; c++)
      if (strchr ("\t\n\r !\"#$&'()*;<=>?^[\\]`{|}~", c) == NULL)
        {
          char s[5];
          s[0] = 'a';
          s[1] = (char) c;
          s[2] = 'z';
          s[3] = (char) c;
          s[4] = '\0';

          check_one (interpreter, prog, s);
        }
  }
}

int
main (int argc, char *argv[])
{
  char *prog;

  if (argc != 2)
    {
      fprintf (stderr, "%s: need 1 argument\n", argv[0]);
      return 2;
    }
  prog = argv[1];

#ifdef WINDOWS_NATIVE
  /* Make PROG suitable for native Windows system calls and cmd.exe:
     Replace '/' with '\\'.  */
  {
    char *p;
    for (p = prog; *p != '\0'; p++)
      if (*p == '/')
        *p = '\\';
  }
#endif

#ifdef WINDOWS_NATIVE
  check_all (SCI_SYSTEM, true, prog); /* equivalent to SCI_WINDOWS_CMD */
  check_all (SCI_WINDOWS_CREATEPROCESS, false, prog);
  check_all (SCI_WINDOWS_CMD, true, prog);
#else
  check_all (SCI_SYSTEM, false, prog); /* equivalent to SCI_POSIX_SH */
#endif

  /* Clean up.  */
  unlink (EXPECTED_DATA_FILE);

  return failed;
}
