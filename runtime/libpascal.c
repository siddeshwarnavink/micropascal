/* ----- libpascal.c begin ----- */

#include <stdio.h>
#include <string.h>

void
_P__p_write_int (int x)
{
  printf ("%d", x);
}
void
_P__p_write_real (double x)
{
  printf ("%g", x);
}
void
_P__p_write_str (const char *s)
{
  printf ("%s", s);
}
void
_P__p_write_char (char c)
{
  putchar (c);
}

/* ----- libpascal.c end ----- */
