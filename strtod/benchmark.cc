#include <getopt.h>
#include <iostream>
#include <ctime>
#include <vector>

#include "timing.hh"
#include "double-conversion/double-conversion.h"
#include "fast_double_parser.h"

extern "C" {

  double xstrtod(
    const char *str, char **endptr, char decimal, char sci, char tsep, 
    int skip_trailing);

  double precise_xstrtod(
    const char *str, char **endptr, char decimal, char sci, char tsep, 
    int skip_trailing);

  double str2dbl(char const*);

  // FIXME: Real signature is char const**.
  double intstrtod(char const*, char**);
  double intstrtod_unrolled1(char const*, char**);
  double intstrtod_unrolled2(char const*, char**);

  double strtod_gay(const char*, char**);

}

//------------------------------------------------------------------------------

inline double
rand(
  double const max)
{
  auto constexpr den = 1.0 / RAND_MAX;
  return random() * den * max;
}


inline char*
rand_arr(
  double const max,
  size_t const num,
  size_t const width)
{
  char* result = new char[num * width];
  for (size_t i = 0; i < num; ++i)
    snprintf(result + i * width, width, "%.8f", rand(max));
  return result;
}


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

inline double
_fast_double_parser(
  char const* s)
{
  double val;
  auto isok = fast_double_parser::parse_number(s, &val);
  assert(isok);
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


extern "C" double parse_double_3(char const*, char const*);
extern "C" double parse_double_4(char const*, char const*);
extern "C" double parse_double_5(char const*, char const*);
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
  size_t width = 10;
  double scale = 1;
  double time = 2;

  char ch;
  while ((ch = getopt_long(
            argc, argv, "n:w:s:t:", long_options, nullptr)) != -1)
    switch (ch) {
    case 'n':
      num = (size_t) atol(optarg);
      break;
    case 'w':
      width = (size_t) atol(optarg);
      break;
    case 's':
      scale = wrap_strtod<strtod>(optarg);
      break;
    case 't':
      time = wrap_strtod<strtod>(optarg);
      break;
    default:
      std::cerr << "invalid usage\n";
      exit(EXIT_FAILURE);
    }

  auto const str_arr = rand_arr(scale, num, width);

  if (false) {
    for (size_t i = 0; i < num; ++i)
      std::cout << &str_arr[i * width] << "\n";
    std::cout << std::flush;
  }

  Timer timer{time, 0.25};
  std::cout 
    << "intstrtod      "
    << " val=" 
    << std::fixed << std::setw(20) << std::setprecision(10)
    << time_fn<wrap_strtod<intstrtod>>(str_arr, width, num)
    << " time: " << timer(time_fn<wrap_strtod<intstrtod>>, str_arr, width, num) / num 
    << std::endl

    << "intstrtod_unr1 "
    << " val=" 
    << std::fixed << std::setw(20) << std::setprecision(10)
    << time_fn<wrap_strtod<intstrtod_unrolled1>>(str_arr, width, num)
    << " time: " << timer(time_fn<wrap_strtod<intstrtod_unrolled1>>, str_arr, width, num) / num 
    << std::endl

    << "intstrtod_unr2 "
    << " val=" 
    << std::fixed << std::setw(20) << std::setprecision(10)
    << time_fn<wrap_strtod<intstrtod_unrolled2>>(str_arr, width, num)
    << " time: " << timer(time_fn<wrap_strtod<intstrtod_unrolled2>>, str_arr, width, num) / num 
    << std::endl

    << "parse_double_3 "
    << " val="
    << std::fixed << std::setw(20) << std::setprecision(10)
    << time_parse_fn<parse_double_3>(str_arr, width, num)
    << " time: " << timer(time_parse_fn<parse_double_3>, str_arr, width, num) / num
    << std::endl

    << "parse_double_4 "
    << " val="
    << std::fixed << std::setw(20) << std::setprecision(10)
    << time_parse_fn<parse_double_4>(str_arr, width, num)
    << " time: " << timer(time_parse_fn<parse_double_4>, str_arr, width, num) / num
    << std::endl

    << "parse_double_5 "
    << " val="
    << std::fixed << std::setw(20) << std::setprecision(10)
    << time_parse_fn<parse_double_5>(str_arr, width, num)
    << " time: " << timer(time_parse_fn<parse_double_5>, str_arr, width, num) / num
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

    << "strtod         "
    << " val=" 
    << std::fixed << std::setw(20) << std::setprecision(10)
    << time_fn<wrap_strtod<strtod>>(str_arr, width, num)
    << " time: " << timer(time_fn<wrap_strtod<strtod>>, str_arr, width, num) / num 
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

    << "fast_double_par"
    << " val=" 
    << std::fixed << std::setw(20) << std::setprecision(10)
    << time_fn<_fast_double_parser>(str_arr, width, num)
    << " time: " << timer(time_fn<_fast_double_parser>, str_arr, width, num) / num 
    << std::endl

    ;

  return EXIT_SUCCESS;
}


