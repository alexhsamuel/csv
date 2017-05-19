#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

int64_t
get_time()
{
  struct timeval tv;
  int rval = gettimeofday(&tv, NULL);
  assert(rval == 0);
  return ((int64_t) 1e6) * tv.tv_sec + tv.tv_usec;
}


extern double str2dbl(char const* s);

int
main()
{
  size_t const n = 1024 * 1024;
  size_t const w = 24;
  char* nums = malloc(w * n);
  memset(nums, 0, w * n);

  for (size_t i = 0; i < n; ++i)
    snprintf(&nums[i * w], w, "%.16lf", random() / 2147483647.0);

  for (int i = 0; i < 8; ++i) {
    int64_t const s0 = get_time();
    double sum = 0;
    for (size_t i = 0; i < n; ++i) {
      char* end;
      sum += strtod(&nums[i * w], &end);
      assert(*end == 0);
    }
    int64_t const e0 = get_time();
    printf("strtod  %lld %f\n", e0 - s0, sum);
  }

  for (int i = 0; i < 8; ++i) {
    int64_t const s0 = get_time();
    double sum = 0;
    for (size_t i = 0; i < n; ++i)
      sum += str2dbl(&nums[i * w]);
    int64_t const e0 = get_time();
    printf("str2dbl %lld %f\n", e0 - s0, sum);
  }

  return 0;
}
