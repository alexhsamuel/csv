#pragma once

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>

#include "json.hh"
#include "util.hh"

using aslib::json::Json;

//------------------------------------------------------------------------------

using Clock = std::chrono::high_resolution_clock;
using Timestamp = std::chrono::time_point<Clock>;
using Elapsed = double;

inline Elapsed
time_diff(
  Timestamp const start,
  Timestamp const end)
{
  return std::chrono::duration<Elapsed>(end - start).count();
}


inline Elapsed
time_since(
  Timestamp const start)
{
  return time_diff(start, Clock::now());
}


/*
 * Invokes `fn(args...)`, and returns the elapsed time and return value.
 */
template<typename FN, typename ...ARGS>
__attribute((noinline))
std::pair<Elapsed, std::result_of_t<FN&&(ARGS&&...)>>
inline time1(
  FN&& fn,
  ARGS&&... args)
{
  auto const start  = Clock::now();
  auto const result = fn(std::forward<ARGS>(args)...);
  auto const end    = Clock::now();
  return {time_diff(start, end), result};
}


//------------------------------------------------------------------------------

template<typename T>
struct SummaryStats
{
  size_t num_samples;
  T min;
  T max;
  T mean;
  T standard_deviation;
};


/*
 * Computes summary statistics over a number of sample values.
 */
template<typename ITER>
auto
summarize(
  ITER begin,
  ITER end)
{
  using Value = typename std::iterator_traits<ITER>::value_type;

  // Compute moments.
  size_t m0 = 0;
  Value m1 = 0;
  Value m2 = 0;
  for (auto i = begin; i < end; ++i) {
    m0 += 1;
    m1 += *i;
    m2 += *i * *i;
  }

  return SummaryStats<Value>{
    m0,
    *begin,      // FIXME: Wrong!
    *(end - 1),  // FIXME: Wrong!
    m1 / m0,
    sqrt(m2 / m0 - square(m1 / m0)) * m0 / (m0 - 1)
  };
}


template<typename T>
inline SummaryStats<T>
operator/(
  SummaryStats<T> const& stats,
  size_t const scale)
{
  return {
    stats.num_samples,
    stats.min / scale,
    stats.max / scale,
    stats.mean / scale,
    stats.standard_deviation / scale
  };
}


template<typename T>
Json
to_json(
  SummaryStats<T> const& stats)
{
  auto json = Json::new_obj();
  json["num_samples"] = Json((int) stats.num_samples);
  json["min"] = stats.min;
  json["max"] = stats.max;
  json["mean"] = stats.mean;
  json["standard_deviation"] = stats.standard_deviation;
  return json;
}
  

inline
std::string
format_ns(
  Elapsed const elapsed)
{
  double const ns = elapsed * 1e9;
  std::stringstream ss;
  ss << std::setw(12) << std::setprecision(2) << std::fixed << ns << " ns";
  return ss.str();
}


inline std::ostream&
operator<<(
  std::ostream& os,
  SummaryStats<Elapsed> const& stats)
{
  os << "n=" << std::setw(4) << stats.num_samples
     << " (" 
     << std::setw(12) << std::setprecision(2) << std::fixed << stats.mean * 1e9 
     << " ± " 
     << std::setw(11) << std::setprecision(3) << stats.standard_deviation * 1e9
     << ") ns";
  return os;
}


//------------------------------------------------------------------------------

class Timer
{
public:

  static size_t constexpr MAX_SAMPLES = 1024;

  using SetupFn = void (*)();

  Timer(
    Elapsed const time_budget,
    double const discard_fraction=0,
    SetupFn const setup=nullptr
    )
  : time_budget_(time_budget),
    discard_fraction_(discard_fraction),
    setup_(setup)
  {
    assert(time_budget > 0);
    assert(discard_fraction >= 0);
  }

  template<typename FN, typename ...ARGS>
  SummaryStats<Elapsed>
  operator()(
    FN&& fn,
    ARGS&&... args)
  {
    std::vector<Elapsed> elapsed;

    auto const start = Clock::now();
    do {
      if (setup_ != nullptr)
        setup_();
      elapsed.push_back(time1(fn, std::forward<ARGS>(args)...).first);
    } while (elapsed.size() < MAX_SAMPLES && time_since(start) < time_budget_);
      
    // Sort and ignore the highest and lowest.
    std::sort(elapsed.begin(), elapsed.end());
    size_t const num_discard = elapsed.size() * discard_fraction_ / 2;
    auto const begin = elapsed.begin() + num_discard;
    auto const end   = elapsed.end()   - num_discard;

    return summarize(begin, end);
  }

private:

  Elapsed const time_budget_;
  double const discard_fraction_;
  SetupFn const setup_;

};


