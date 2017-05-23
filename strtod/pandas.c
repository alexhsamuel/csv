// From pandas.

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <math.h>

// ---------------------------------------------------------------------------
// Implementation of xstrtod

//
// strtod.c
//
// Convert string to double
//
// Copyright (C) 2002 Michael Ringgaard. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the project nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//
// -----------------------------------------------------------------------
// Modifications by Warren Weckesser, March 2011:
// * Rename strtod() to xstrtod().
// * Added decimal and sci arguments.
// * Skip trailing spaces.
// * Commented out the other functions.
// Modifications by Richard T Guy, August 2013:
// * Add tsep argument for thousands separator
//

double xstrtod(const char *str, char **endptr, char decimal, char sci,
               char tsep, int skip_trailing) {
    double number;
    int exponent;
    int negative;
    char *p = (char *)str;
    double p10;
    int n;
    int num_digits;
    int num_decimals;

    errno = 0;

    // Skip leading whitespace.
    while (isspace(*p)) p++;

    // Handle optional sign.
    negative = 0;
    switch (*p) {
        case '-':
            negative = 1;  // Fall through to increment position.
        case '+':
            p++;
    }

    number = 0.;
    exponent = 0;
    num_digits = 0;
    num_decimals = 0;

    // Process string of digits.
    while (isdigit(*p)) {
        number = number * 10. + (*p - '0');
        p++;
        num_digits++;

        p += (tsep != '\0' && *p == tsep);
    }

    // Process decimal part.
    if (*p == decimal) {
        p++;

        while (isdigit(*p)) {
            number = number * 10. + (*p - '0');
            p++;
            num_digits++;
            num_decimals++;
        }

        exponent -= num_decimals;
    }

    if (num_digits == 0) {
        errno = ERANGE;
        return 0.0;
    }

    // Correct for sign.
    if (negative) number = -number;

    // Process an exponent string.
    if (toupper(*p) == toupper(sci)) {
        // Handle optional sign.
        negative = 0;
        switch (*++p) {
            case '-':
                negative = 1;  // Fall through to increment pos.
            case '+':
                p++;
        }

        // Process string of digits.
        num_digits = 0;
        n = 0;
        while (isdigit(*p)) {
            n = n * 10 + (*p - '0');
            num_digits++;
            p++;
        }

        if (negative)
            exponent -= n;
        else
            exponent += n;

        // If no digits, after the 'e'/'E', un-consume it
        if (num_digits == 0) p--;
    }

    if (exponent < DBL_MIN_EXP || exponent > DBL_MAX_EXP) {
        errno = ERANGE;
        return HUGE_VAL;
    }

    // Scale the result.
    p10 = 10.;
    n = exponent;
    if (n < 0) n = -n;
    while (n) {
        if (n & 1) {
            if (exponent < 0)
                number /= p10;
            else
                number *= p10;
        }
        n >>= 1;
        p10 *= p10;
    }

    if (number == HUGE_VAL) {
        errno = ERANGE;
    }

    if (skip_trailing) {
        // Skip trailing whitespace.
        while (isspace(*p)) p++;
    }

    if (endptr) *endptr = p;

    return number;
}

double precise_xstrtod(const char *str, char **endptr, char decimal, char sci,
                       char tsep, int skip_trailing) {
    double number;
    int exponent;
    int negative;
    char *p = (char *)str;
    int num_digits;
    int num_decimals;
    int max_digits = 17;
    int n;
    // Cache powers of 10 in memory.
    static double e[] = {
        1.,    1e1,   1e2,   1e3,   1e4,   1e5,   1e6,   1e7,   1e8,   1e9,
        1e10,  1e11,  1e12,  1e13,  1e14,  1e15,  1e16,  1e17,  1e18,  1e19,
        1e20,  1e21,  1e22,  1e23,  1e24,  1e25,  1e26,  1e27,  1e28,  1e29,
        1e30,  1e31,  1e32,  1e33,  1e34,  1e35,  1e36,  1e37,  1e38,  1e39,
        1e40,  1e41,  1e42,  1e43,  1e44,  1e45,  1e46,  1e47,  1e48,  1e49,
        1e50,  1e51,  1e52,  1e53,  1e54,  1e55,  1e56,  1e57,  1e58,  1e59,
        1e60,  1e61,  1e62,  1e63,  1e64,  1e65,  1e66,  1e67,  1e68,  1e69,
        1e70,  1e71,  1e72,  1e73,  1e74,  1e75,  1e76,  1e77,  1e78,  1e79,
        1e80,  1e81,  1e82,  1e83,  1e84,  1e85,  1e86,  1e87,  1e88,  1e89,
        1e90,  1e91,  1e92,  1e93,  1e94,  1e95,  1e96,  1e97,  1e98,  1e99,
        1e100, 1e101, 1e102, 1e103, 1e104, 1e105, 1e106, 1e107, 1e108, 1e109,
        1e110, 1e111, 1e112, 1e113, 1e114, 1e115, 1e116, 1e117, 1e118, 1e119,
        1e120, 1e121, 1e122, 1e123, 1e124, 1e125, 1e126, 1e127, 1e128, 1e129,
        1e130, 1e131, 1e132, 1e133, 1e134, 1e135, 1e136, 1e137, 1e138, 1e139,
        1e140, 1e141, 1e142, 1e143, 1e144, 1e145, 1e146, 1e147, 1e148, 1e149,
        1e150, 1e151, 1e152, 1e153, 1e154, 1e155, 1e156, 1e157, 1e158, 1e159,
        1e160, 1e161, 1e162, 1e163, 1e164, 1e165, 1e166, 1e167, 1e168, 1e169,
        1e170, 1e171, 1e172, 1e173, 1e174, 1e175, 1e176, 1e177, 1e178, 1e179,
        1e180, 1e181, 1e182, 1e183, 1e184, 1e185, 1e186, 1e187, 1e188, 1e189,
        1e190, 1e191, 1e192, 1e193, 1e194, 1e195, 1e196, 1e197, 1e198, 1e199,
        1e200, 1e201, 1e202, 1e203, 1e204, 1e205, 1e206, 1e207, 1e208, 1e209,
        1e210, 1e211, 1e212, 1e213, 1e214, 1e215, 1e216, 1e217, 1e218, 1e219,
        1e220, 1e221, 1e222, 1e223, 1e224, 1e225, 1e226, 1e227, 1e228, 1e229,
        1e230, 1e231, 1e232, 1e233, 1e234, 1e235, 1e236, 1e237, 1e238, 1e239,
        1e240, 1e241, 1e242, 1e243, 1e244, 1e245, 1e246, 1e247, 1e248, 1e249,
        1e250, 1e251, 1e252, 1e253, 1e254, 1e255, 1e256, 1e257, 1e258, 1e259,
        1e260, 1e261, 1e262, 1e263, 1e264, 1e265, 1e266, 1e267, 1e268, 1e269,
        1e270, 1e271, 1e272, 1e273, 1e274, 1e275, 1e276, 1e277, 1e278, 1e279,
        1e280, 1e281, 1e282, 1e283, 1e284, 1e285, 1e286, 1e287, 1e288, 1e289,
        1e290, 1e291, 1e292, 1e293, 1e294, 1e295, 1e296, 1e297, 1e298, 1e299,
        1e300, 1e301, 1e302, 1e303, 1e304, 1e305, 1e306, 1e307, 1e308};
    errno = 0;

    // Skip leading whitespace.
    while (isspace(*p)) p++;

    // Handle optional sign.
    negative = 0;
    switch (*p) {
        case '-':
            negative = 1;  // Fall through to increment position.
        case '+':
            p++;
    }

    number = 0.;
    exponent = 0;
    num_digits = 0;
    num_decimals = 0;

    // Process string of digits.
    while (isdigit(*p)) {
        if (num_digits < max_digits) {
            number = number * 10. + (*p - '0');
            num_digits++;
        } else {
            ++exponent;
        }

        p++;
        p += (tsep != '\0' && *p == tsep);
    }

    // Process decimal part
    if (*p == decimal) {
        p++;

        while (num_digits < max_digits && isdigit(*p)) {
            number = number * 10. + (*p - '0');
            p++;
            num_digits++;
            num_decimals++;
        }

        if (num_digits >= max_digits)  // Consume extra decimal digits.
            while (isdigit(*p)) ++p;

        exponent -= num_decimals;
    }

    if (num_digits == 0) {
        errno = ERANGE;
        return 0.0;
    }

    // Correct for sign.
    if (negative) number = -number;

    // Process an exponent string.
    if (toupper(*p) == toupper(sci)) {
        // Handle optional sign
        negative = 0;
        switch (*++p) {
            case '-':
                negative = 1;  // Fall through to increment pos.
            case '+':
                p++;
        }

        // Process string of digits.
        num_digits = 0;
        n = 0;
        while (isdigit(*p)) {
            n = n * 10 + (*p - '0');
            num_digits++;
            p++;
        }

        if (negative)
            exponent -= n;
        else
            exponent += n;

        // If no digits after the 'e'/'E', un-consume it.
        if (num_digits == 0) p--;
    }

    if (exponent > 308) {
        errno = ERANGE;
        return HUGE_VAL;
    } else if (exponent > 0) {
        number *= e[exponent];
    } else if (exponent < -308) {  // Subnormal
        if (exponent < -616)       // Prevent invalid array access.
            number = 0.;
        number /= e[-308 - exponent];
        number /= e[308];
    } else {
        number /= e[-exponent];
    }

    if (number == HUGE_VAL || number == -HUGE_VAL) errno = ERANGE;

    if (skip_trailing) {
        // Skip trailing whitespace.
        while (isspace(*p)) p++;
    }

    if (endptr) *endptr = p;
    return number;
}

