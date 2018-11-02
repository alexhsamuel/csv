#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" double parse_double_7(char const*, char const*);

int
main(
  int const argc,
  char const* const* const argv)
{
  if (argc == 2) {
    auto const start = argv[1];
    auto const end = start + strlen(start);
    auto const value = parse_double_7(start, end);
    printf("res: %20.17e\n", value);
    printf("ref: %20.17e\n", strtod(start, nullptr));
    return 0;
  }
  else {
    fprintf(stderr, "usage: %s NUMBER\n", argv[0]);
    return 1;
  }
}
