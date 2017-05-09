#include <cassert>
#include <fcntl.h>
#include <iostream>
#include <limits>
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


std::ostream&
operator<<(
  std::ostream& os,
  Buffer const& buf)
{
  for (char const* p = buf.ptr; p < buf.ptr + buf.len; ++p)
    std::cout << *p;
  return os;
}


//------------------------------------------------------------------------------

class Column
{
public:

  using size_type = std::vector<char>::size_type;

  Column() { offsets_.push_back(0); }

  Column(
    size_type const len) 
  : offsets_(len, MISSING) 
  { 
    offsets_.push_back(0); 
  }

  size_type size() const { return offsets_.size() - 1; }

  bool 
  started() 
    const 
  { 
    return offsets_.back() > chars_.size();
  }

  void append(char c) { chars_.push_back(c); }
  void finish() { offsets_.push_back(chars_.size()); }

  void 
  missing() 
  { 
    offsets_.back() |= MISSING; 
    finish(); 
  }

  Buffer 
  operator[](
    size_type const idx)
    const
  {
    assert(idx < size());
    auto const off = offsets_[idx];
    if (off & MISSING) 
      return {nullptr, 0};
    else {
      auto const len = (offsets_[idx + 1] & ~MISSING) - off;
      return {&chars_[off], len};
    }
  }

private:

  static constexpr auto MISSING = size_type(1) << (sizeof(size_type) * 8 - 1);

  std::vector<char> chars_;
  std::vector<size_type> offsets_;

};


Column::size_type const Column::MISSING;


std::vector<Column>
split_columns(
  Buffer const& buffer,
  char const sep=',',
  char const eol='\n',
  char const quote='"')
{
  std::vector<Column> cols;

  if (buffer.len == 0)
    return cols;

  std::vector<Column>::size_type col_idx = 0;
  cols.emplace_back();

  for (size_t i = 0; i < buffer.len; ++i) {
    char const c = buffer.ptr[i];

    if (c == eol) {
      // End the field.
      cols[col_idx].finish();

      // Remaining colums are missing in this row.
      for (++col_idx; col_idx < cols.size(); ++col_idx) {
        cols[col_idx].missing();
      }

      // End the line.
      col_idx = 0;
    }
    else if (c == sep) {
      // End the field.
      cols[col_idx].finish();

      ++col_idx;
      if (col_idx >= cols.size())
        // Create a new column, and back-fill it with missing.
        cols.emplace_back(cols[0].size() - 1);
    }
    // Fast-forward through quoted strings.
    else if (c == quote) {
      // Skip over the opening quote.
      i += 1;
      for (; i < buffer.len && buffer.ptr[i] != quote; ++i)
        cols[col_idx].append(buffer.ptr[i]);
      // FIXME: else: unclosed quote.
    }
    else
      cols[col_idx].append(c);
        
    // FIXME: Trailing field?
  }

  // Remaining colums are missing in this row.
  if (cols[0].started())
    for (; col_idx < cols.size(); ++col_idx)
      cols[col_idx].missing();

  return cols;
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
  
  void* ptr = mmap(nullptr, info.st_size, PROT_READ, MAP_SHARED, fd, 0);
  assert(ptr != MAP_FAILED);

  Buffer buf{static_cast<char const*>(ptr), (size_t) info.st_size};

  // std::cout << buf;

  auto const cols = split_columns(buf);

  for (auto const col : cols) {
    std::cout << "COLUMN " << col.size() << "\n";
    for (Column::size_type i = 0; i < col.size(); ++i) {
      std::cout << i << '.' << ' ';
      if (col[i].ptr == nullptr)
        std::cout << "missing";
      else
        std::cout << col[i];
      std::cout << '\n';
    }
    std::cout << '\n';
  }

  // FIXME: munmap.
  // FIXME: close.

  return 0;
}


