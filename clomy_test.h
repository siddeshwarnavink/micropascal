/* clomy.h v1.0 - C library of my own - Siddeshwar

   A tiny assertion library that provides Vim's Quickfix friendly error
   message.

   To use this library:
     #include "clomy_test.h"

   To add "clomy_" namespace prefix:
     #define CLOMY_NO_SHORT_NAMES

   To remove all assertion:
     #define CLOMY_TEST_DISABLE

   To learn how to use this library I would recommend checking tests/ codes. */

#ifndef CLOMY_TEST_H
#define CLOMY_TEST_H

#include <stdio.h>

#ifndef CLOMY_TEST_DISABLE

/* Utility to print error message. */
#define CLOMY_FAIL(msg)                                                       \
  do                                                                          \
    {                                                                         \
      fprintf (stderr, "%s:%d:0: Assertion failed: %s\n", __FILE__, __LINE__, \
               (msg));                                                        \
      exit (1);                                                               \
    }                                                                         \
  while (0);

/* Utility to print error message if EXP is false. */
#define CLOMY_FAILFALSE(exp, msg)                                             \
  if (!(exp))                                                                 \
    CLOMY_FAIL ((msg));

/* Utility to print error message if EXP is true. */
#define CLOMY_FAILTRUE(exp, msg)                                             \
  if ((exp))                                                                 \
  CLOMY_FAIL ((msg));

#else

#define CLOMY_FAIL
#define CLOMY_FAILFALSE
#define CLOMY_FAILTRUE

#endif /* not CLOMY_TEST_DISABLE */

#ifndef CLOMY_NO_SHORT_NAMES

#define FAIL CLOMY_FAIL
#define FAILFALSE CLOMY_FAILFALSE
#define FAILTRUE CLOMY_FAILTRUE

#endif /* not CLOMY_NO_SHORT_NAMES */

#endif /* not CLOMY_TEST_H */
