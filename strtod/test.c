#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern double parse_double_6(char const*, char const*);

int
main(
  int argc,
  char const* const* const argv)
{
  if (argc != 2) {
    fprintf(stderr, "usage: %s NUMBER\n", argv[0]);
    return EXIT_FAILURE;
  }
  double const d = parse_double_6(argv[1], argv[1] + strlen(argv[1]));
  double const r = strtod(argv[1], NULL);
  printf("parse_double -> %.20f = %.20e\n", d, d);
  printf("strtod       -> %.20f = %.20e\n", r, r);
  return EXIT_SUCCESS;
}


