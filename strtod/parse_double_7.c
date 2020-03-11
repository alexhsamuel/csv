#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>  // FIXME
#include <stdint.h>
#include <stdio.h>  // FIXME

#include "parse_double.h"

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

//------------------------------------------------------------------------------

static int const pd7_debug = 0;

static inline long
pow5(
  int const i)
{
  static long const powers[] = {
                     1,
                     5,
                    25,
                   125,
                   625,
                  3125,
                 15625,
                 78125,
                390625,
               1953125,
               9765625,
              48828125,
             244140625,
            1220703125,
            6103515625,
           30517578125,
          152587890625,
          762939453125,
         3814697265625,
        19073486328125,
        95367431640625,
       476837158203125,
      2384185791015625,
     11920928955078125,
     59604644775390625,
    298023223876953125,
   1490116119384765625,
   7450580596923828125,
  };
  assert(0 <= i && i < sizeof(powers) / sizeof(long));
  return powers[i];
}


//------------------------------------------------------------------------------

union double_bits {
  double val;
  struct {
    unsigned long mant : 52;
    unsigned int  exp  : 11;
    unsigned int  sign :  1;
  } __attribute__ ((__packed__)) bits;
};


inline double shift_exp(
  double const val,
  int shift2)
{
  union double_bits d;
  d.val = val;
  d.bits.exp += shift2;
  return d.val;
}


union int128_bits {
  unsigned __int128 val;
  unsigned long parts[2];
};


inline void
print128(
  unsigned __int128 const val)
{
  union int128_bits b;
  b.val = val;
  printf("%016lx %016lx", b.parts[1], b.parts[0]);
}


inline int clz128(
  unsigned __int128 const x)
{
  unsigned long const hi = x >> 64;
  return hi == 0 ? 64 + __builtin_clzl(x) : __builtin_clzl(hi);
}


static inline void
show(
  char const* const label,
  unsigned __int128 const mant,
  int exp2)
{
  printf("%s: mant=", label);
  print128(mant);
  double const val = (double) mant * pow(2, exp2);
  printf(" exp2=%4d val=%20.17e\n", exp2, val);
}


static double 
scale(
  unsigned long const base,
  int const exp10)
{
  if (pd7_debug)
    printf("scale(base=%016lx, exp10=%3d)\n", base, exp10);

  if (unlikely(base == 0))
    return 0;

  if (unlikely(exp10 == 0))
    return base;

  // Double mantissa and exponent.
  unsigned __int128 mant;
  int exp2;

  // The strategy is to scale by powers of 10 by scaling the mantissa by powers
  // of 5, which requires an unsigned long multiplication or division, and
  // simply tracking the remaining powers of 2.  The powers of 2 are applied
  // directly to the double exponent.

  if (exp10 > 0) {
    // Check if the value multiplied by 5s fits in unsigned long.  If it does,
    // we can multiply, perform hardware conversion directly to double, and 
    // shift the double exponent by 2s.  Note that 149/64 is just a bit larger 
    // than log2(5).
    if (likely(149 * exp10 / 64 + 1 <= __builtin_clzl(base)))
      return shift_exp(base * pow5(exp10), exp10);

    // Doesn't fit in unsigned long; use unsigned __int128.
    mant = base;
    // Multiply mantissa by 5s; track 2s in the exponent.
    mant = (unsigned __int128) base * pow5(exp10);
    exp2 = exp10;
  }

  else {
    // We shift the base as far as we can into unsigned __int128, to maximize
    // precision in division.
    int const shift = 64 + __builtin_clzl(base);
    mant = (unsigned __int128) base << shift;
    if (pd7_debug) {
      show("start", base, 0);
      printf("shift=%d\n", shift);
      show("shift", mant, -shift);
    }
    // Multiply mantissa by 5s; track 2s in the exponent.
    mant /= pow5(-exp10);
    exp2 = exp10 - shift;
    if (pd7_debug)
      show("div10", mant, exp2);
  }

  // Now build the normalized 52-bit mantissa.
  unsigned long mant52;

  // Shift the mantissa to normalized position with MSB in bit 53.
  int const bits = 128 - clz128(mant);
  if (likely(bits > 53)) {
    // Shift the mantissa right, with rounding, while increasing the exponent.
    int const shift = bits - 54;
    if (pd7_debug)
      printf("shift=%3d\n", shift);
    // Round up based the first bit we'll shift off.
    mant52 = mant >> shift;
    exp2 += shift;
    if (pd7_debug)
      show("nrm52", mant52, exp2);
    mant52 = (mant52 + 1) >> 1;
    exp2 += 1;
    if (pd7_debug)
      show("round", mant52, exp2);
  }
  else if (likely(bits < 53)) {
    // Shift the mantissa left while reducing the exponent.
    int const shift = bits - 53;
    mant52 = (unsigned long) mant << shift;
    exp2 -= shift;
  }
  else
    // Perfect; nothing to do.
    mant52 = mant;

  // Build the double.  Drop bit 53 of the mantissa; it's implied.
#if 0
  union double_bits result;
  result.val = 0;
  if (pd7_debug)
    printf("result=%016lx\n", *(unsigned long*) &result.val);
  result.bits.exp = exp2 + 1023 + 52;
  if (pd7_debug)
    printf("result=%016lx\n", *(unsigned long*) &result.val);
  result.bits.sign = 0;
  result.bits.mant = mant52 & 0xfffffffffffffl;
  if (pd7_debug)
    printf("result=%016lx\n", *(unsigned long*) &result.val);
  return result.val;
#else
  unsigned long result =
    (unsigned long) (exp2 + 1023 + 52) << 52
    | (mant52 & 0xfffffffffffffl);
  if (pd7_debug)
    printf("result=%016lx\n", result);
  return *(double*) &result;
#endif
}


double
parse_double_7(
  char const* const ptr,
  char const* const end)
{
  char const* p = ptr;
  int exp = 0;

  // Parse a sign, if present.
  int negative = 0;
  if (*p == '-') {
    negative = 1;
    ++p;
  }
  else if (*p == '+')
    ++p;

  int digits = INT_MIN;
  long val = 0;

  // Next should be either a digit or a decimal point.
  if (unlikely(p >= end))
    return NAN_PARSE_ERROR;
  else {
    int const d = *p - '0';
    if (likely(0 <= d && d <= 9)) {
      val = d;
      ++digits;  // FIXME: Unnecessary?
    }
    else if (likely(d == '.' - '0' && digits < 0))
      digits = 0;
    else
      return NAN_PARSE_ERROR;
  }

  // Unroll parsing additional characters.  Each may be a digit, or a decimal
  // point if one hasn't been seen already, or an 'E', which indicates
  // E-notation.

#define next_digit(pos)                                                     \
  if (unlikely(pos + p >= end))                                             \
    goto end;                                                               \
  else {                                                                    \
    int const d = p[pos] - '0';                                             \
    if (likely(0 <= d && d <= 9)) {                                         \
      val = val * 10 + d;                                                   \
      ++digits;                                                             \
    }                                                                       \
    else if (likely(d == '.' - '0' && digits < 0))                          \
      digits = 0;                                                           \
    else if (likely(d == 'e' - '0' || d == 'E' - '0')) {                    \
      p += pos + 1;                                                         \
      goto exp;                                                             \
    }                                                                       \
    else                                                                    \
      return NAN_PARSE_ERROR;                                               \
  }

  next_digit( 1);
  next_digit( 2);
  next_digit( 3);
  next_digit( 4);
  next_digit( 5);
  next_digit( 6);
  next_digit( 7);
  next_digit( 8);
  next_digit( 9);
  next_digit(10);
  next_digit(11);
  next_digit(12);
  next_digit(13);
  next_digit(14);
  next_digit(15);
  next_digit(16);
  next_digit(17);
  p += 18;

#undef next_digit

  // FIXME: If we still haven't seen a decimal point, the number is inf.

  // Process any remaining digits.
  while (unlikely(p < end)) {
    int const d = *p++ - '0';
    if (likely(0 <= d && d <= 9))
      // Remaining digits don't matter; insufficient precision.
      exp += 1;
    else if (likely(d == 'e' - '0' || d == 'E' - '0')) 
      goto exp;
    else if (d == -48)  // FIXME: This should not be needed.
      break;
    else
      return NAN_PARSE_ERROR;
  }

end:
  exp += digits < 0 ? 0 : -digits;
  double const result = scale(val, exp);
  return negative ? -result : result;

exp:;
  // Got an 'E'; parse the exponent of scientific E-notation.

  // Parse the exponent sign, if any.
  int exp_negative = 0;
  if (*p == '-') {
    exp_negative = 1;
    ++p;
  }
  else if (*p == '+')
    ++p;

  // We partially unroll the loop.  Parse the first exponential digit.
  if (unlikely(p == end))
    // No digits-- missing exponent.
    return NAN_PARSE_ERROR;
  else {
    int const d = *p++ - '0';
    if (likely(0 <= d && d <= 9))
      exp = d;
    else
      return NAN_PARSE_ERROR;
  }

  // Parse the second exponential digit.
  if (p == end)
    goto exp_end;
  else {
    int const d = *p++ - '0';
    if (likely(0 <= d && d <= 9))
      exp = 10 * exp + d;
    else
      return NAN_PARSE_ERROR;
  }

  // Parse the third and subsequent exponential digits.
  while (p < end) {
    int const d = *p++ - '0';
    if (likely(0 <= d && d <= 9))
      exp = 10 * exp + d;
    else
      return NAN_PARSE_ERROR;
  }

exp_end:
  // End of the exponent.
  if (exp_negative) 
    exp = -exp;
  goto end;

}


