#include <stddef.h>
static const char *
find_conversion (const char  *format,
                 const char **after)
{
  const char *start = format;
  const char *cp;

  while (*start != '\0' && *start != '%')
    start++;

  if (*start == '\0')
    {
      *after = start;
      return NULL;
    }

  cp = start + 1;

  if (*cp == '\0')
    { 
      *after = cp;
      return NULL;
    }

  /* Test for positional argument.  */
  if (*cp >= '0' && *cp <= '9')
    {
      const char *np;

      for (np = cp; *np >= '0' && *np <= '9'; np++)
        ;
      if (*np == '$')
        cp = np + 1;
    }

  /* Skip the flags.  */
  for (;;)
    {
      if (*cp == '\'' ||
          *cp == '-' ||
          *cp == '+' ||
          *cp == ' ' ||
          *cp == '#' ||
          *cp == '0')
        cp++;
      else
        break;
    }

  /* Skip the field width.  */
  if (*cp == '*')
    {
      cp++;

      /* Test for positional argument.  */
      if (*cp >= '0' && *cp <= '9')
        {
          const char *np;

          for (np = cp; *np >= '0' && *np <= '9'; np++);
          if (*np == '$')
            cp = np + 1;
        }
    }
  else
    {
      for (; *cp >= '0' && *cp <= '9'; cp++)
        ;
    }

  /* Skip the precision.  */
  if (*cp == '.')
    {
      cp++;
      if (*cp == '*')
        {
          /* Test for positional argument.  */ if (*cp >= '0' && *cp <= '9')
            {
              const char *np;

              for (np = cp; *np >= '0' && *np <= '9'; np++)
                ;
              if (*np == '$')
                cp = np + 1;
            }
        }
      else
        {
          for (; *cp >= '0' && *cp <= '9'; cp++)
            ;
        }
    }

  /* Skip argument type/size specifiers.  */
  while (*cp == 'h' ||
         *cp == 'L' ||
         *cp == 'l' ||
         *cp == 'j' ||
        *cp == 'z' ||
         *cp == 'Z' ||
         *cp == 't')
    cp++;

  /* Skip the conversion character.  */
  cp++;

  *after = cp;
  return start;
}

