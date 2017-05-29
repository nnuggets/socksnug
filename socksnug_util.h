#ifndef SOCKSNUG_UTIL_H
#define SOCKSNUG_UTIL_H

#include <assert.h>

#ifdef DEBUG
#define SN_ASSERT(expr) assert(expr)
#else
#define SN_ASSERT(expr)
#endif /* DEBUG */

#define SN_PACKED __attribute__((__packed__))

int is_unsigned_int(char *s);

#define SN_EXIT_IF_TRUE(cond, s)		\
  do {						\
    if ( cond ) {				\
      perror("perror");				\
      fprintf(stderr, s "\n");			\
      exit(EXIT_FAILURE);			\
    }						\
  } while ( 0 )

#define SN_RETURN_IF_TRUE(cond, s, ret_value)	\
  do {						\
    if ( cond ) {				\
      perror("perror");				\
      fprintf(stderr, s "\n");			\
      return (ret_value);			\
    }						\
  } while ( 0 )

#endif /* SOCKSNUG_UTIL_H */
