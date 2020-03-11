#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

inline int
same_double(
  double const f1,
  double const f2)
{
  return *((int64_t*) &f1) == *((int64_t*) &f2);
}


#define NAN_PARSE_ERROR (__builtin_nan("0x7ff8dce5318cebb6"))


inline int
is_parse_error(
  double const val)
{
  return same_double(val, NAN_PARSE_ERROR);
}


extern double parse_double_6(char const*, char const*);
extern double parse_double_7(char const*, char const*);


#ifdef __cplusplus
}  // extern "C"
#endif

