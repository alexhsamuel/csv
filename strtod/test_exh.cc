#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <random>

#include "parse_double.h"


auto rng = std::mt19937_64();
auto digit_dist = std::uniform_int_distribution<int>(48, 57);
auto bool_dist = std::uniform_int_distribution<int>(0, 1);


inline int
generate_exp(
  int const num_dig,
  int exp,
  char* buf)
{
  int i = 0;
  buf[i++] = (char) digit_dist(rng);
  buf[i++] = '.';
  for (i = 2; i < num_dig + 1; ++i)
    buf[i] = (char) digit_dist(rng);
  buf[i++] = 'e';
  if (exp < 0) {
    buf[i++] = '-';
    exp = -exp;
  }
  if (exp >= 100) {
    buf[i++] = '0' + exp / 100;
    exp %= 100;
  }
  if (exp >= 10) {
    buf[i++] = '0' + exp / 10;
    exp %= 10;
  }
  buf[i++] = '0' + exp;
  return i;
}

void
test(
  int const num_dig,
  int const exp,
  int const count)
{
  char buf[512];
  for (int c = 0; c < count; ++c) {
    auto const len = generate_exp(num_dig, exp, buf);
    buf[len] = 0;
    char* end;
    auto const ref = strtod(buf, &end);
    assert(end == buf + len);
    auto const val = parse_double_7(buf, buf + len);
    assert(!is_parse_error(val));
    if (!same_double(val, ref))
      printf("%20.17e %20.17e '%s'\n", ref, val, buf);
  }
}


int
main(
  int const argc,
  char const* const* const argv)
{
  test(6, 0, 10000);
  return 0;
}

