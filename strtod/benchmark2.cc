#include <ctime>
#include <getopt.h>
#include <iostream>
#include <random>
#include <vector>

#include "timing.hh"
#include "double-conversion/double-conversion.h"

extern "C" {

  double xstrtod(
    const char *str, char **endptr, char decimal, char sci, char tsep, 
    int skip_trailing);

  double precise_xstrtod(
    const char *str, char **endptr, char decimal, char sci, char tsep, 
    int skip_trailing);

  double str2dbl(char const*);

  double strtod_gay(const char*, char**);

}

//------------------------------------------------------------------------------

namespace rnd {

static std::default_random_engine
rng;

static std::uniform_int_distribution<int>
digit_dist(48, 57);

inline char
digit() {
  return digit_dist(rng);
}

inline char*
double_arr(
  size_t const num,
  size_t const width,
  size_t const dec)
{
  char* arr = new char[num * width];
  for (size_t i = 0; i < num; ++i) {
    for (size_t j = 0; j < width - 1; ++j)
     arr[i * width + j] = j == dec ? '.' : digit();
    arr[i * width + width - 1] = 0;
  }
  return arr;
}


}  // namespace rnd


template<double (*F)(char const*, char**)>
inline double
wrap_strtod(
  char const* s)
{
  char* end;
  auto const val = F(s, &end);
  assert(*end == 0);
  return val;
}


inline double
_xstrtod(
  char const* s)
{
  char* end;
  auto const val = xstrtod(s, &end, '.', 'e', 0, 0);
  assert(*end == 0); 
  return val;
}


inline double
_precise_xstrtod(
  char const* s)
{
  char* end;
  auto const val = precise_xstrtod(s, &end, '.', 'e', 0, 0);
  assert(*end == 0);
  return val;
}


static double_conversion::StringToDoubleConverter const 
S2DC{
  double_conversion::StringToDoubleConverter::ALLOW_TRAILING_JUNK,
  -100,
  -101,
  "infinity",
  "NaN",
  double_conversion::StringToDoubleConverter::kNoSeparator
};

inline double
_StringToDoubleConverter(
  char const* start,
  char const* end)
{
  int count = -1;
  auto const val = S2DC.StringToDouble(start, end - start, &count);
  assert(count > 0);
  return val;
}


using strtod_type = double (*)(char const*);

template<strtod_type FN>
inline double
time_fn(
  char const* const arr,
  size_t const width,
  size_t const num)
{
  double sum = 0;
  for (size_t i = 0; i < num; ++i)
    sum += FN(arr + i * width);
  return sum;
}


extern inline int
same_double(
  double const f1,
  double const f2)
{
  return *((int64_t*) &f1) == *((int64_t*) &f2);
}


double const NAN_PARSE_ERROR = __builtin_nan("0x7ff8dce5318cebb6");


extern inline int
is_parse_error(
  double const val)
{
  return same_double(val, NAN_PARSE_ERROR);
}


extern "C" double parse_double_6(char const*, char const*);
extern "C" double parse_double_7(char const*, char const*);

using parse_double_type = double (*)(char const*, char const*);

template<parse_double_type FN>
inline double
time_parse_fn(
  char const* const arr,
  size_t const width,
  size_t const num)
{
  double sum = 0;
  for (size_t i = 0; i < num; ++i) {
    double const val = FN(arr + i * width, arr + i * width + width - 1);
    assert(!is_parse_error(val));
    sum += val;
  }
  return sum;
}
  


int
main(
  int const argc,
  char* const* const argv)
{
#if defined(BSD)
  srandomdev();
#eise
  srandom(time(nullptr));
#endif

  struct option const long_options[] = {
    {"number", required_argument, nullptr, 'n'},
    {"width", required_argument, nullptr, 'w'},
    {"scale", required_argument, nullptr, 's'},
    {"time", required_argument, nullptr, 't'},
    {nullptr, 0, nullptr, 0}
  };

  size_t num = 1024 * 1024;
  size_t width = 8;
  double dec = 1;
  double time = 2;
  bool show_numbers = false;

  char ch;
  while ((ch = getopt_long(
            argc, argv, "n:w:d:t:S", long_options, nullptr)) != -1)
    switch (ch) {
    case 'n':
      num = (size_t) atol(optarg);
      break;
    case 'w':
      width = (size_t) atol(optarg);
      break;
    case 'd':
      dec = wrap_strtod<strtod>(optarg);
      break;
    case 't':
      time = wrap_strtod<strtod>(optarg);
      break;
    case 'S':
      show_numbers = true;
      break;
    default:
      std::cerr << "invalid usage\n";
      exit(EXIT_FAILURE);
    }

  auto const str_arr = rnd::double_arr(num, width, dec);

  if (show_numbers) {
    for (size_t i = 0; i < num; ++i)
      std::cout << &str_arr[i * width] << "\n";
    std::cout << std::flush;
  }

  Timer timer{time, 0.25};
  std::cout 
    << "strtod         "
    << " val=" 
    << std::fixed << std::setw(20) << std::setprecision(10)
    << time_fn<wrap_strtod<strtod>>(str_arr, width, num)
    << " time: " << timer(time_fn<wrap_strtod<strtod>>, str_arr, width, num) / num 
    << std::endl

    << "parse_double_6 "
    << " val="
    << std::fixed << std::setw(20) << std::setprecision(10)
    << time_parse_fn<parse_double_6>(str_arr, width, num)
    << " time: " << timer(time_parse_fn<parse_double_6>, str_arr, width, num) / num
    << std::endl

    << "parse_double_7 "
    << " val="
    << std::fixed << std::setw(20) << std::setprecision(10)
    << time_parse_fn<parse_double_7>(str_arr, width, num)
    << " time: " << timer(time_parse_fn<parse_double_7>, str_arr, width, num) / num
    << std::endl

    << "xstrtod        "
    << " val=" 
    << std::fixed << std::setw(20) << std::setprecision(10)
    << time_fn<_xstrtod>(str_arr, width, num)
    << " time: " << timer(time_fn<_xstrtod>, str_arr, width, num) / num 
    << std::endl

    << "precise_xstrtod"
    << " val=" 
    << std::fixed << std::setw(20) << std::setprecision(10)
    << time_fn<_precise_xstrtod>(str_arr, width, num)
    << " time: " << timer(time_fn<_precise_xstrtod>, str_arr, width, num) / num 
    << std::endl

    << "str2dbl        "
    << " val=" 
    << std::fixed << std::setw(20) << std::setprecision(10)
    << time_fn<str2dbl>(str_arr, width, num)
    << " time: " << timer(time_fn<str2dbl>, str_arr, width, num) / num 
    << std::endl

    << "strtod_gay     "
    << " val=" 
    << std::fixed << std::setw(20) << std::setprecision(10)
    << time_fn<wrap_strtod<strtod_gay>>(str_arr, width, num)
    << " time: " << timer(time_fn<wrap_strtod<strtod_gay>>, str_arr, width, num) / num 
    << std::endl

    << "StringToDoubleC"
    << " val=" 
    << std::fixed << std::setw(20) << std::setprecision(10)
    << time_parse_fn<_StringToDoubleConverter>(str_arr, width, num)
    << " time: " << timer(time_parse_fn<_StringToDoubleConverter>, str_arr, width, num) / num 
    << std::endl

    ;

  return EXIT_SUCCESS;
}


