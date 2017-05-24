#include <getopt.h>
#include <iostream>
#include <vector>

#include "timing.hh"

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
    snprintf(result + i * width, width, "%.18f", rand(max));
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


int
main(
  int const argc,
  char* const* const argv)
{
  srandomdev();

  struct option const long_options[] = {
    {"number", required_argument, nullptr, 'n'},
    {"width", required_argument, nullptr, 'w'},
    {"scale", required_argument, nullptr, 's'},
    {nullptr, 0, nullptr, 0}
  };

  size_t num = 1024 * 1024;
  size_t width = 32;
  double scale = 1;

  char ch;
  while ((ch = getopt_long(
            argc, argv, "n:w:s:", long_options, nullptr)) != -1)
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

  Timer timer{2, 0.25};
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
    ;

  return EXIT_SUCCESS;
}


