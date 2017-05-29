#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern double parse_double_5(char const*, char const*);

int
main(
  int argc,
  char const* const* const argv)
{
  if (argc != 2) {
    fprintf(stderr, "usage: %s NUMBER\n", argv[0]);
    return EXIT_FAILURE;
  }
  double const d = parse_double_5(argv[1], argv[1] + strlen(argv[1]));
  printf("%.16e\n", d);
  return EXIT_SUCCESS;
}

