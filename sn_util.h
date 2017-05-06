#ifndef SOCKSNUG_UTIL_H
#define SOCKSNUG_UTIL_H

#include <assert.h>

#ifdef DEBUG
#define SN_ASSERT(expr) assert(expr)
#else
#define SN_ASSERT(expr) 
#endif /* DEBUG */

#define SN_PACKED __attribute__((__packed__))

#endif /* SOCKSNUG_UTIL_H */
