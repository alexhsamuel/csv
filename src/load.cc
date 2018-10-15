#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "ThreadPool.hh"
#include "csv2.hh"

//------------------------------------------------------------------------------

void
load_file(
  char const* const path)
{
  int const fd = open(path, O_RDONLY);
  assert(fd >= 0);

  struct stat info;
  int res = fstat(fd, &info);
  if (res != 0) {
    perror("fstat");
    exit(EXIT_FAILURE);
  }
  assert(res == 0);
  std::cout << "st_size = " << info.st_size << std::endl;
  
  void* ptr = mmap(nullptr, info.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (ptr == MAP_FAILED) {
    perror("mmap");
    exit(EXIT_FAILURE);
  }

  Buffer buf{static_cast<char const*>(ptr), (size_t) info.st_size};

  auto const cols = split_columns(buf);
  std::cerr << "done with split\n";

  {
    ThreadPool pool(5);
    std::vector<std::future<Array>> results;
    // FIXME: Use std::transform.
    for (auto const& col : cols)
      results.push_back(pool.enqueue(parse_array, &col, true));

    for (auto&& result : results)
      std::cout << result.get();
  }

  // FIXME: munmap.
  // FIXME: close.
}


