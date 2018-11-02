#include <stdio.h>
#include <stdlib.h>

#include "parse_double.h"


inline double
_rand(
  double const max)
{
  static double const den = 1.0 / RAND_MAX;
  return random() * den * max;
}


inline char*
rand_arr(
  double const max,
  size_t const num,
  size_t const width)
{
  char* result = malloc(num * width);
  for (size_t i = 0; i < num; ++i)
    snprintf(result + i * width, width, "%.8f", _rand(max));
  return result;
}


int
main()
{
  int const width = 32;
  int const num = 10000000;
  char* arr = rand_arr(1.0, num, width);
  double total = 0;
  for (int i = 0; i < num; ++i) {
    char const* start = arr + i * width;
    char const* end = start + 10;
    total += parse_double_7(start, end);
  }
  return (int) total;
}

