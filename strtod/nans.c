#include "math.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"

//------------------------------------------------------------------------------

typedef union {
  double value;
  unsigned char bytes[sizeof(double)];
} float64;


void
print(
  float64 f)
{
  printf("%25.15f =", f.value);
  for (int i = 0; i < sizeof(f.bytes); ++i)
    printf(" %02x", f.bytes[i]);
  printf(" = %016lx\n", *((unsigned long*) &f));
}


extern inline int
same_double(
  double const f1,
  double const f2)
{
  return *((int64_t*) &f1) == *((int64_t*) &f2);
}


double const NAN_PARSE_ERROR = __builtin_nan("0x7ff8dce5318cebb6");


extern inline int
is_parse_error(
  double const val)
{
  return same_double(val, NAN_PARSE_ERROR);
}


int
main()
{
  float64 f;

  f.value = 0;
  print(f);

  f.value = 1;
  print(f);

  f.value = 42;
  print(f);

  f.value = NAN;
  print(f);

  f.value = nan("42");
  print(f);

  f.value = nan("12345");
  print(f);

  f.value = NAN_PARSE_ERROR;
  print(f);

  double f1 = NAN_PARSE_ERROR;
  printf("==  ? %s\n", f1 == f1 ? "yes" : "no");
  printf("same? %s\n", same_double(f1, f1) ? "yes" : "no");
  printf("same? %s\n", same_double(f1, nan("12345")) ? "yes" : "no");
  printf("same? %s\n", is_parse_error(f1) ? "yes" : "no");
  
  return EXIT_SUCCESS;
}
