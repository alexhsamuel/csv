#include <cstring>

#include "strtod/parse_double.h"
#include "gtest/gtest.h"

//------------------------------------------------------------------------------

inline double
parse6(
  char const* const ptr)
{
  return parse_double_6(ptr, ptr + strlen(ptr));
}


TEST(basic, parse_double_6) {
  EXPECT_EQ(parse6("1.0"), 1.0);
}

