#include <stdio.h>
#include <stdlib.h>

extern double strtod_gay(const char* s00, char** se);

int main(int const argc, char const* const* const argv) {
  if (argc != 2) {
    fprintf(stderr, "usage: %s NUMBER\n", argv[0]);
    exit(1);
  }

  char* end = NULL;
  double val = strtod_gay(argv[1], &end);
  if (end == NULL)
    printf("end = NULL\n");
  else
    printf("parsed %ld\n", end - argv[1]);
  printf("value = %.18le\n", val);

  return 0;
}

