#include <stdio.h>
#include <stdlib.h>

extern double str2dbl(char const* s);

int
main(
  int argc,
  char const* const* const argv)
{
  if (argc != 2) {
    fprintf(stderr, "usage: %s NUMBER\n", argv[0]);
    return EXIT_FAILURE;
  }
  double const d = str2dbl(argv[1]);
  printf("%.16e\n", d);
  return EXIT_SUCCESS;
}
