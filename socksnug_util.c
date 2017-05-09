#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "socksnug.h"
#include "socksnug_util.h"

int is_unsigned_int(char *s) {
  int i = 0;

  SN_ASSERT( s != NULL );

  if ( s[i] != '+' ) {
    if ( !isdigit(s[i]) )
      return 0;
  } else if ( !isdigit(s[++i]) ) {
    return 0;
  }

  while ( isdigit(s[++i]) );

  return s[i] == '\0';
}
