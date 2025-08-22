#include "utils.h"

U8
streq (const char *a, const char *b)
{
  while (*a && *b)
    {
      if (*a != *b)
        return false;
      a++;
      b++;
    }
  return *a == *b;
}
