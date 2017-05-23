#include <stdio.h>
#include <stdlib.h>

extern double intstrtod(char const*, char const**);

int
main(
  int const argc,
  char const* const* const argv)
{
  char const* end;
  double const val = intstrtod(argv[1], &end);
  printf("parsed %ld chars, *end=%d\n", end - argv[1], (int) *end);
  printf("%32.15e\n", val);
  return EXIT_SUCCESS;
}

