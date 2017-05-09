#include <cassert>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <vector>

// FIXME: UTF-8 (and other encodings?)

//------------------------------------------------------------------------------

struct Buffer
{
  char const* ptr;
  size_t len;
};


struct Field
{
  size_t off;
  size_t len;
};


struct Split
{
  std::vector<Field> fields;
  std::vector<size_t> lines;
};


Buffer
slice(
  Buffer const& buf,
  Field const& field)
{
  return {buf.ptr + field.off, field.len};
}


std::ostream&
operator<<(
  std::ostream& os,
  Buffer const& buf)
{
  for (char const* p = buf.ptr; p < buf.ptr + buf.len; ++p)
    std::cout << *p;
  return os;
}


Split
find_split(
  Buffer const& buffer,
  char const sep=',',
  char const eol='\n',
  char const quote='"')
{
  Split split;

  size_t off = 0;
  bool in_quote = false;

  for (size_t i = 0; i < buffer.len; ++i) {
    char const c = buffer.ptr[i];
    if (c == quote) 
      in_quote = !in_quote;
    else if (! in_quote && (c == sep || c == eol)) {
      // End the field.
      split.fields.push_back({off, i - off});
      off = i + 1;
      if (c == eol) 
        split.lines.push_back(split.fields.size());
    }
    // FIXME: Trailing field?
  }
  // FIXME: Trailing line?

  return split;
}


int
main(
  int const argc,
  char const* const* const argv)
{
  int const fd = open(argv[1], O_RDONLY);
  assert(fd >= 0);

  struct stat info;
  int res = fstat(fd, &info);
  assert(res == 0);
  std::cout << "st_size = " << info.st_size << std::endl;
  
  void* ptr = 
    mmap(nullptr, info.st_size, PROT_READ, MAP_FILE | MAP_SHARED, fd, 0);
  assert(ptr != MAP_FAILED);

  Buffer buf{static_cast<char const*>(ptr), (size_t) info.st_size};
  std::cout << buf;

  auto const split = find_split(buf);
  size_t i = 0;
  for (auto const line : split.lines) {
    for (; i < line; ++i) {
      auto const& field = split.fields[i];
      // std::cout << field.off << '+' << field.len << ' ';
      std::cout << '[' << slice(buf, field) << ']';
    }
    std::cout << '\n';
  }

  return 0;
}


