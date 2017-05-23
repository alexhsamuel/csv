#pragma once

#include <cstddef>
#include <string>
#include <utility>

//------------------------------------------------------------------------------

template<typename T> T square(T val) { return val * val; }

/*
 * Calls fn on args and returns the result, suppressing inlining.
 */
template<typename FN, typename ...ARGS>
__attribute((noinline)) std::result_of_t<FN&&(ARGS&&...)>
no_inline(
  FN&& fn,
  ARGS&&... args)
{
  return fn(std::forward<ARGS>(args)...);
}


//------------------------------------------------------------------------------

/*
 * Parses a size.
 *
 * The size may be a bare integer, or an integer followed by a
 * (case-insensitive) 'k', 'm', or 'g' suffix to indicate 1024-based scale.
 */
extern long parse_size(std::string const&);

extern void thrash_cache(size_t);

