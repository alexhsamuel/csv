#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>

#include "parse_double.h"

#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

//------------------------------------------------------------------------------

static inline double
pow10(
  int const i)
{
  static double const powers[] = {  1E-308, 1E-307, 1E-306, 1E-305, 
    1E-304, 1E-303, 1E-302, 1E-301, 1E-300, 1E-299, 1E-298, 1E-297, 
    1E-296, 1E-295, 1E-294, 1E-293, 1E-292, 1E-291, 1E-290, 1E-289, 
    1E-288, 1E-287, 1E-286, 1E-285, 1E-284, 1E-283, 1E-282, 1E-281, 
    1E-280, 1E-279, 1E-278, 1E-277, 1E-276, 1E-275, 1E-274, 1E-273, 
    1E-272, 1E-271, 1E-270, 1E-269, 1E-268, 1E-267, 1E-266, 1E-265, 
    1E-264, 1E-263, 1E-262, 1E-261, 1E-260, 1E-259, 1E-258, 1E-257, 
    1E-256, 1E-255, 1E-254, 1E-253, 1E-252, 1E-251, 1E-250, 1E-249, 
    1E-248, 1E-247, 1E-246, 1E-245, 1E-244, 1E-243, 1E-242, 1E-241, 
    1E-240, 1E-239, 1E-238, 1E-237, 1E-236, 1E-235, 1E-234, 1E-233, 
    1E-232, 1E-231, 1E-230, 1E-229, 1E-228, 1E-227, 1E-226, 1E-225, 
    1E-224, 1E-223, 1E-222, 1E-221, 1E-220, 1E-219, 1E-218, 1E-217, 
    1E-216, 1E-215, 1E-214, 1E-213, 1E-212, 1E-211, 1E-210, 1E-209, 
    1E-208, 1E-207, 1E-206, 1E-205, 1E-204, 1E-203, 1E-202, 1E-201, 
    1E-200, 1E-199, 1E-198, 1E-197, 1E-196, 1E-195, 1E-194, 1E-193, 
    1E-192, 1E-191, 1E-190, 1E-189, 1E-188, 1E-187, 1E-186, 1E-185, 
    1E-184, 1E-183, 1E-182, 1E-181, 1E-180, 1E-179, 1E-178, 1E-177, 
    1E-176, 1E-175, 1E-174, 1E-173, 1E-172, 1E-171, 1E-170, 1E-169, 
    1E-168, 1E-167, 1E-166, 1E-165, 1E-164, 1E-163, 1E-162, 1E-161, 
    1E-160, 1E-159, 1E-158, 1E-157, 1E-156, 1E-155, 1E-154, 1E-153, 
    1E-152, 1E-151, 1E-150, 1E-149, 1E-148, 1E-147, 1E-146, 1E-145, 
    1E-144, 1E-143, 1E-142, 1E-141, 1E-140, 1E-139, 1E-138, 1E-137, 
    1E-136, 1E-135, 1E-134, 1E-133, 1E-132, 1E-131, 1E-130, 1E-129, 
    1E-128, 1E-127, 1E-126, 1E-125, 1E-124, 1E-123, 1E-122, 1E-121, 
    1E-120, 1E-119, 1E-118, 1E-117, 1E-116, 1E-115, 1E-114, 1E-113, 
    1E-112, 1E-111, 1E-110, 1E-109, 1E-108, 1E-107, 1E-106, 1E-105, 
    1E-104, 1E-103, 1E-102, 1E-101, 1E-100, 1E-099, 1E-098, 1E-097, 
    1E-096, 1E-095, 1E-094, 1E-093, 1E-092, 1E-091, 1E-090, 1E-089, 
    1E-088, 1E-087, 1E-086, 1E-085, 1E-084, 1E-083, 1E-082, 1E-081, 
    1E-080, 1E-079, 1E-078, 1E-077, 1E-076, 1E-075, 1E-074, 1E-073, 
    1E-072, 1E-071, 1E-070, 1E-069, 1E-068, 1E-067, 1E-066, 1E-065, 
    1E-064, 1E-063, 1E-062, 1E-061, 1E-060, 1E-059, 1E-058, 1E-057, 
    1E-056, 1E-055, 1E-054, 1E-053, 1E-052, 1E-051, 1E-050, 1E-049, 
    1E-048, 1E-047, 1E-046, 1E-045, 1E-044, 1E-043, 1E-042, 1E-041, 
    1E-040, 1E-039, 1E-038, 1E-037, 1E-036, 1E-035, 1E-034, 1E-033, 
    1E-032, 1E-031, 1E-030, 1E-029, 1E-028, 1E-027, 1E-026, 1E-025, 
    1E-024, 1E-023, 1E-022, 1E-021, 1E-020, 1E-019, 1E-018, 1E-017, 
    1E-016, 1E-015, 1E-014, 1E-013, 1E-012, 1E-011, 1E-010, 1E-009, 
    1E-008, 1E-007, 1E-006, 1E-005, 1E-004, 1E-003, 1E-002, 1E-001, 
    1E+000, 1E+001, 1E+002, 1E+003, 1E+004, 1E+005, 1E+006, 1E+007, 
    1E+008, 1E+009, 1E+010, 1E+011, 1E+012, 1E+013, 1E+014, 1E+015, 
    1E+016, 1E+017, 1E+018, 1E+019, 1E+020, 1E+021, 1E+022, 1E+023, 
    1E+024, 1E+025, 1E+026, 1E+027, 1E+028, 1E+029, 1E+030, 1E+031, 
    1E+032, 1E+033, 1E+034, 1E+035, 1E+036, 1E+037, 1E+038, 1E+039, 
    1E+040, 1E+041, 1E+042, 1E+043, 1E+044, 1E+045, 1E+046, 1E+047, 
    1E+048, 1E+049, 1E+050, 1E+051, 1E+052, 1E+053, 1E+054, 1E+055, 
    1E+056, 1E+057, 1E+058, 1E+059, 1E+060, 1E+061, 1E+062, 1E+063, 
    1E+064, 1E+065, 1E+066, 1E+067, 1E+068, 1E+069, 1E+070, 1E+071, 
    1E+072, 1E+073, 1E+074, 1E+075, 1E+076, 1E+077, 1E+078, 1E+079, 
    1E+080, 1E+081, 1E+082, 1E+083, 1E+084, 1E+085, 1E+086, 1E+087, 
    1E+088, 1E+089, 1E+090, 1E+091, 1E+092, 1E+093, 1E+094, 1E+095, 
    1E+096, 1E+097, 1E+098, 1E+099, 1E+100, 1E+101, 1E+102, 1E+103, 
    1E+104, 1E+105, 1E+106, 1E+107, 1E+108, 1E+109, 1E+110, 1E+111, 
    1E+112, 1E+113, 1E+114, 1E+115, 1E+116, 1E+117, 1E+118, 1E+119, 
    1E+120, 1E+121, 1E+122, 1E+123, 1E+124, 1E+125, 1E+126, 1E+127, 
    1E+128, 1E+129, 1E+130, 1E+131, 1E+132, 1E+133, 1E+134, 1E+135, 
    1E+136, 1E+137, 1E+138, 1E+139, 1E+140, 1E+141, 1E+142, 1E+143, 
    1E+144, 1E+145, 1E+146, 1E+147, 1E+148, 1E+149, 1E+150, 1E+151, 
    1E+152, 1E+153, 1E+154, 1E+155, 1E+156, 1E+157, 1E+158, 1E+159, 
    1E+160, 1E+161, 1E+162, 1E+163, 1E+164, 1E+165, 1E+166, 1E+167, 
    1E+168, 1E+169, 1E+170, 1E+171, 1E+172, 1E+173, 1E+174, 1E+175, 
    1E+176, 1E+177, 1E+178, 1E+179, 1E+180, 1E+181, 1E+182, 1E+183, 
    1E+184, 1E+185, 1E+186, 1E+187, 1E+188, 1E+189, 1E+190, 1E+191, 
    1E+192, 1E+193, 1E+194, 1E+195, 1E+196, 1E+197, 1E+198, 1E+199, 
    1E+200, 1E+201, 1E+202, 1E+203, 1E+204, 1E+205, 1E+206, 1E+207, 
    1E+208, 1E+209, 1E+210, 1E+211, 1E+212, 1E+213, 1E+214, 1E+215, 
    1E+216, 1E+217, 1E+218, 1E+219, 1E+220, 1E+221, 1E+222, 1E+223, 
    1E+224, 1E+225, 1E+226, 1E+227, 1E+228, 1E+229, 1E+230, 1E+231, 
    1E+232, 1E+233, 1E+234, 1E+235, 1E+236, 1E+237, 1E+238, 1E+239, 
    1E+240, 1E+241, 1E+242, 1E+243, 1E+244, 1E+245, 1E+246, 1E+247, 
    1E+248, 1E+249, 1E+250, 1E+251, 1E+252, 1E+253, 1E+254, 1E+255, 
    1E+256, 1E+257, 1E+258, 1E+259, 1E+260, 1E+261, 1E+262, 1E+263, 
    1E+264, 1E+265, 1E+266, 1E+267, 1E+268, 1E+269, 1E+270, 1E+271, 
    1E+272, 1E+273, 1E+274, 1E+275, 1E+276, 1E+277, 1E+278, 1E+279, 
    1E+280, 1E+281, 1E+282, 1E+283, 1E+284, 1E+285, 1E+286, 1E+287, 
    1E+288, 1E+289, 1E+290, 1E+291, 1E+292, 1E+293, 1E+294, 1E+295, 
    1E+296, 1E+297, 1E+298, 1E+299, 1E+300, 1E+301, 1E+302, 1E+303, 
    1E+304, 1E+305, 1E+306, 1E+307, 1E+308
  };

  assert(-308 <= i && i <= 308);
  return powers[i - -308];
}


//------------------------------------------------------------------------------

double
parse_double_6(
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
      // Remaining digits don't matter.
      ;
    else if (likely(d == 'e' - '0' || d == 'E' - '0')) 
      goto exp;
    else if (d == -48)  // FIXME: This should not be needed.
      break;
    else
      return NAN_PARSE_ERROR;
  }
  
end:
  exp += digits < 0 ? 0 : -digits;
  if (negative)
    val = -val;
  return exp == 0 ? val : val * pow10(exp);

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


