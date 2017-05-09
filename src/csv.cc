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
  size_t off;  // byte offset into buffer
  size_t len;  // number of bytes in field
};


struct Split
{
  struct Line
  {
    size_t idx;  // index of first field in fields
    size_t num;  // number of fields in line
  };

  std::vector<Field> fields;
  std::vector<Line> lines;
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
  size_t field_idx = 0;
  size_t num_fields = 0;

  for (size_t i = 0; i < buffer.len; ++i) {
    char const c = buffer.ptr[i];

    // Look for a quote at the beginning of a field, then fast-forward.
    if (c == quote && off == i) {
      for (i += 1; i < buffer.len && buffer.ptr[i] != quote; ++i)
        ;
      // FIXME: Check for unclosed quote.
    }

    if (c == sep || c == eol) {
      // End the field.
      split.fields.push_back({off, i - off});
      ++num_fields;
      off = i + 1;
      if (c == eol) {
        // End the line.
        split.lines.push_back({field_idx, num_fields});
        field_idx = split.fields.size();
        num_fields = 0;
      }
    }
    // FIXME: Trailing field?
  }
  split.lines.push_back({field_idx, num_fields});

  return split;
}


//------------------------------------------------------------------------------

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
  for (auto const line : split.lines) {
    for (auto i = line.idx; i < line.idx + line.num; ++i) {
      auto const& field = split.fields[i];
      // std::cout << field.off << '+' << field.len << ' ';
      std::cout << '[' << slice(buf, field) << ']';
    }
    std::cout << '\n';
  }

  return 0;
}


