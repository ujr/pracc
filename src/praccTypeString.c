#include "pracc.h"

/**
 * Translate a pracc record type to its name.
 * Also useful to test record types for validity.
 *
 * Return a one-word description of the type
 * or NULL if the given type is invalid.
 */
char *
praccTypeString(char type)
{
   if (type == '-') return "debit";
   if (type == '+') return "credit";
   if (type == '=') return "reset";
   if (type == '$') return "limit";
   if (type == '!') return "error";
   if (type == '#') return "note";

   return (char *) 0; // invalid type!
}
