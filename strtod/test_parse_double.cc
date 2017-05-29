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


TEST(integers, parse_double_6) {
  EXPECT_EQ(parse6(          "0"),           0.0);
  EXPECT_EQ(parse6(       "0001"),           1.0);
  EXPECT_EQ(parse6(          "1"),           1.0);
  EXPECT_EQ(parse6(         "10"),          10.0);
  EXPECT_EQ(parse6(        "010"),          10.0);
  EXPECT_EQ(parse6(    "1000000"),     1000000.0);
  EXPECT_EQ(parse6( "8275002873"),  8275002873.0);

  EXPECT_EQ(parse6(         "-0"),           0.0);
  EXPECT_EQ(parse6(      "-0001"),          -1.0);
  EXPECT_EQ(parse6(         "-1"),          -1.0);
  EXPECT_EQ(parse6(        "-10"),         -10.0);
  EXPECT_EQ(parse6(       "-010"),         -10.0);
  EXPECT_EQ(parse6(   "-1000000"),    -1000000.0);
  EXPECT_EQ(parse6("-8275002873"), -8275002873.0);
}

TEST(no_integer, parse_double_6) {
  EXPECT_EQ(parse6(".125"), 0.125);
}

TEST(huge, parse_double_6) {
  EXPECT_EQ(parse6("100000000000000000000000000000000"), 1e+32);
}

