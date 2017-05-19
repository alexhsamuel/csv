#include <cassert>
#include <fcntl.h>
#include <iostream>
#include <limits>
#include <experimental/optional>
#include <sys/mman.h>
#include <sys/stat.h>
#include <vector>

using std::experimental::optional;

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

// FIXME: Compute max width in column?

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

  Column(Column const&) = default;
  Column(Column&&) = default;
  Column& operator=(Column const&) = default;
  Column& operator=(Column&&) = default;

  inline size_type size() const { return offsets_.size() - 1; }

  inline bool 
  started() 
    const 
  { 
    return offsets_.back() > chars_.size();
  }

  inline void append(char c) { chars_.push_back(c); }
  inline void finish() { offsets_.push_back(chars_.size()); }

  inline void 
  missing() 
  { 
    offsets_.back() |= MISSING; 
    finish(); 
  }

  inline Buffer 
  operator[](
    size_type const idx)
    const
  {
    auto const off = offsets_.at(idx);
    if (off & MISSING) 
      return {nullptr, 0};
    else {
      auto const off1 = (offsets_[idx + 1] & ~MISSING);
      if (off1 == off)
        // Empty buffer.
        return {nullptr, 0};
      else {
        assert(off1 <= chars_.size());
        return {&chars_.at(off), off1 - off};
      }
    }
  }

  inline size_type
  max_width()
    const
  {
    size_type max = 0;
    for (    
      auto i0 = offsets_.cbegin(), i1 = i0 + 1;
      i1 != offsets_.cend(); 
      ++i0, ++i1)
      max = std::max(max, (*i1 & ~MISSING) - (*i0 & ~MISSING));
    return max;
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
  if (buffer.len == 0)
    return {};

  std::vector<Column> cols;

  std::vector<Column>::size_type col_idx = 0;
  cols.emplace_back();

  for (size_t i = 0; i < buffer.len; ++i) {
    char const c = buffer.ptr[i];

    if (c == eol) {
      // End the field.
      cols.at(col_idx++).finish();

      // Remaining colums are missing in this row.
      for (; col_idx < cols.size(); ++col_idx)
        cols.at(col_idx).missing();

      // End the line.
      col_idx = 0;
    }
    else if (c == sep) {
      // End the field.
      cols.at(col_idx).finish();

      ++col_idx;
      if (col_idx >= cols.size())
        // Create a new column, and back-fill it with missing.
        cols.emplace_back(cols[0].size() - 1);
    }
    else if (c == quote)
      // Fast-forward through quoted strings: skip over the opening quote, and
      // copy characters until the closing quote.
      for (++i; i < buffer.len && buffer.ptr[i] != quote; ++i)
        cols.at(col_idx).append(buffer.ptr[i]);
      // FIXME: else: unclosed quote.
    else
      cols.at(col_idx).append(c);

    // FIXME: Trailing field?
  }

  // Remaining colums are missing in this row.
  if (col_idx > 0 || cols.at(0).started())
    for (; col_idx < cols.size(); ++col_idx)
      cols.at(col_idx).missing();

  return cols;
}


//------------------------------------------------------------------------------

struct Arr
{
  enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STR,
  } 
  type              = TYPE_INT;

  int64_t int_min   = std::numeric_limits<int64_t>::min();
  int64_t int_max   = std::numeric_limits<int64_t>::max();
  double float_max  = std::numeric_limits<double>::max();
  size_t str_len    = 0;

  // FIXME: union?
  int64_t* int_arr  = nullptr;
  double* float_arr = nullptr;
  char* str_arr     = nullptr;

};


//------------------------------------------------------------------------------

struct StrArr
{
  size_t len;
  size_t width;
  std::vector<char> chars;
};


StrArr
parse_str_arr(
  Column const& col)
{
  auto const len = col.size();
  if (len == 0)
    return StrArr{0, 0, {}};

  // Compute the column width, which is the longest string length.
  auto const width = col.max_width();
  std::vector<char> chars(len * width, 0);
  char* base = chars.data();

  for (Column::size_type i = 0; i < len; ++i) {
    auto const field = col[i];
    memcpy(base + i * width, field.ptr, field.len);
  }

  return {len, width, std::move(chars)};
}


//------------------------------------------------------------------------------

inline bool oadd(uint64_t a, uint64_t b, uint64_t& r) { return __builtin_uaddll_overflow(a, b, &r); }
inline bool oadd( int64_t a,  int64_t b,  int64_t& r) { return __builtin_saddll_overflow(a, b, &r); }
inline bool omul(uint64_t a, uint64_t b, uint64_t& r) { return __builtin_umulll_overflow(a, b, &r); }
inline bool omul( int64_t a,  int64_t b,  int64_t& r) { return __builtin_smulll_overflow(a, b, &r); }

inline optional<uint64_t>
parse_uint64(
  Buffer const buf)
{
  if (buf.len == 0)
    return {};

  // FIXME: Accept leading +?

  uint64_t val = 0;
  for (auto p = buf.ptr; p < buf.ptr + buf.len; ++p) {
    auto const c = *p;
    if ('0' <= c && c <= '9') {
      if (omul(val, 10, val) || oadd(val, c - '0', val))
        return {};
    }
    else
      return {};
  }

  return val;
}


struct UInt64Arr
{
  using vals_type = std::vector<uint64_t>;

  size_t len;
  uint64_t min;
  uint64_t max;
  vals_type vals;
};


inline optional<UInt64Arr>
parse_uint64_arr(
  Column const& col)
{
  auto const len = col.size();
  if (len == 0)
    return UInt64Arr{0, 0, 0, {}};

  uint64_t min;
  uint64_t max;
  UInt64Arr::vals_type vals(len, 0);

  for (Column::size_type i = 0; i < len; ++i) {
    auto const field = col[i];
    auto const val = parse_uint64(field);
    if (val) {
      vals.at(i) = *val;
      if (i == 0)
        min = max = *val;
      else {
        if (*val < min)
          min = *val;
        if (*val > max)
          max = *val;
      }
    }
    else
      return {};
  }

  return UInt64Arr{len, min, max, std::move(vals)};
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

  for (auto const& col : cols) {
    // Try an int array.
    auto const int_arr = parse_uint64_arr(col);
    if (int_arr) {
      std::cout << "uint64 column len=" << int_arr->len
                << " min=" << int_arr->min << " max=" << int_arr->max << "\n";
      for (size_t i = 0; i < int_arr->len; ++i)
        std::cout << i << '.' << ' ' << int_arr->vals[i] << '\n';
    }

    // Fall back to a string array.
    else {
      auto const arr = parse_str_arr(col);
      std::cout << "str column len=" << arr.len
                << " width=" << arr.width << '\n';
      for (size_t i = 0; i < arr.len; ++i) {
        std::cout << i << '.' << ' ' << '[';
        char const* base = arr.chars.data();
        for (char const* p = base + i * arr.width;
             p < base + (i + 1) * arr.width;
             ++p)
          if (*p == 0)
            std::cout << "·";
          else
            std::cout << *p;
        // std::cout << &arr.chars[i * arr.width];
        std::cout << ']' << '\n';
      }
    }

    std::cout << '\n';
  }

  // FIXME: munmap.
  // FIXME: close.

  return 0;
}


