#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <limits>
#include <experimental/optional>
#include <sys/mman.h>
#include <sys/stat.h>
#include <vector>

using std::experimental::optional;

// FIXME: UTF-8 (and other encodings?)

// FIXME: static assert that this is right.
using float64_t = double;

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

// FIXME: Instead of this complicated nonsense with special bits, just provide
// an iterator interface.

// FIXME: Instead of copying values, store offsets into shmem.  Mark whether
// unquoting/unescaping is required.

// FIXME: Track whether there are empty fields (or min width).

// FIXME: Track whether there are non-int chars, non-float chars.

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

  Column(Column const&)             = default;
  Column(Column&&)                  = default;
  Column& operator=(Column const&)  = default;
  Column& operator=(Column&&)       = default;

  inline size_type size() const { return offsets_.size() - 1; }

  inline bool 
  started() 
    const 
  { 
    return offsets_.back() > chars_.size();
  }

  inline void append(char c) { chars_.push_back(c); }

  inline void
  finish()
  {
    auto const off = chars_.size();
    auto const len = off - offsets_.back() & ~MISSING;
    offsets_.push_back(off);
    if (max_width_ < len)
      max_width_ = len;
    if (len == 0)
      has_empty_ = true;
  }

  inline void 
  missing() 
  { 
    has_missing_ = true;
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

  inline bool has_missing() const { return has_missing_; }
  inline bool has_empty() const { return has_empty_; }

  inline size_type
  max_width()
    const
  {
    return max_width_;
  }

private:

  static constexpr auto MISSING = size_type(1) << (sizeof(size_type) * 8 - 1);

  std::vector<char> chars_;
  std::vector<size_type> offsets_;

  bool has_missing_ = false;
  bool has_empty_ = false;
  size_type max_width_ = 0;

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

template<class T> inline optional<T> parse(Buffer const buf);

template<>
inline optional<uint64_t>
parse<uint64_t>(
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


template<>
inline optional<int64_t>
parse<int64_t>(
  Buffer const buf)
{
  if (buf.len == 0)
    // Empty string.
    return {};

  auto p = buf.ptr;
  auto const end = buf.ptr + buf.len;
  bool negative = false;
  if (p[0] == '-') {
    negative = true;
    ++p;
  }
  else if (p[0] == '+')
    ++p;

  if (p == end)
    // No digits.
    return {};

  int64_t val = 0;
  while (p < end) {
    auto const c = *p++;
    if ('0' <= c && c <= '9') {
      if (omul(val, 10, val) || oadd(val, negative ? '0' - c : c - '0', val))
        // Overflow.
        return {};
    }
    else
      // Not a digit.
      return {};
  }

  return val;
}


template<>
inline optional<float64_t>
parse<float64_t>(
  Buffer const buf)
{
  if (buf.len == 0)
    // Empty string.
    return {};

  // FIXME: This is terrible.
  char string[buf.len + 1];
  memcpy(string, buf.ptr, buf.len);
  string[buf.len] = 0;

  char* endptr;
  auto const val = strtod(string, &endptr);
  if (endptr == &string[buf.len])
    return val;
  else
    return {};
}


//------------------------------------------------------------------------------

template<class T>
struct NumberArr
{
  using vals_type = std::vector<T>;

  size_t len;
  T min;
  T max;
  vals_type vals;
  std::string name;
};


template<class T> using Parse = optional<T>(*)(Buffer);

template<class T, Parse<T> PARSE=parse<T>>
inline optional<NumberArr<T>>
parse_number_arr(
  Column const& col,
  bool const header=true)
{
  auto const len = col.size();
  if (len == 0)
    // FIXME: What if header is true?
    return NumberArr<T>{0, 0, 0, {}, {}};

  std::string name;
  Column::size_type i = 0;
  if (header) {
    auto const& n = col[i++];
    name = std::string{n.ptr, n.len};
  }

  // Parse the first value.
  auto const val = PARSE(col[i++]);
  if (!val)
    return {};

  // Initialize min and max.
  T min = *val;
  T max = *val;

  // Allocate space for results.
  typename NumberArr<T>::vals_type vals;
  vals.reserve(len);
  vals.push_back(*val);

  for (; i < len; ++i) {
    auto const field = col[i];
    auto const val = PARSE(field);
    if (val) {
      vals.push_back(*val);
      if (*val < min)
        min = *val;
      if (*val > max)
        max = *val;
    }
    else
      return {};
  }

  assert(vals.size() == len - (header ? 1 : 0));
  return NumberArr<T>{len, min, max, std::move(vals), name};
}


//------------------------------------------------------------------------------

// FIXME: Use a respectable variant.

class Array
{
public:

  using variant_type = enum {
    VARIANT_INT,
    VARIANT_FLOAT,
    VARIANT_STRING,
  };

  Array(NumberArr<int64_t>&& arr) : variant_{VARIANT_INT}, int_arr_{std::move(arr)} {}
  Array(NumberArr<float64_t>&& arr) : variant_{VARIANT_FLOAT}, float_arr_{std::move(arr)} {}
  Array(StrArr&& arr) : variant_{VARIANT_STRING}, str_arr_{std::move(arr)} {}

  variant_type variant() const { return variant_; }

  NumberArr<int64_t> const& int_arr() const { return *int_arr_; }
  NumberArr<float64_t> const& float_arr() const { return *float_arr_; }
  StrArr const& str_arr() const { return *str_arr_; }

private:

  variant_type variant_;

  optional<NumberArr<int64_t>> const int_arr_;
  optional<NumberArr<float64_t>> const float_arr_;
  optional<StrArr> const str_arr_;

};


void
print(
  std::ostream& os,
  Array const& arr,
  bool const print_values=false)
{
  if (arr.variant() == Array::VARIANT_INT) {
    auto const& int_arr = arr.int_arr();
    os << "int64 column '" << int_arr.name 
       << "' len=" << int_arr.len
       << " min=" << int_arr.min << " max=" << int_arr.max << "\n";
    if (print_values)
      for (size_t i = 0; i < int_arr.len; ++i)
        os << i << '.' << ' ' << int_arr.vals[i] << '\n';
  }

  else if (arr.variant() == Array::VARIANT_FLOAT) {
    auto const& float_arr = arr.float_arr();
    os << "float64 column '" << float_arr.name 
       << "' len=" << float_arr.len
       << " min=" << float_arr.min
       << " max=" << float_arr.max << "\n";
    if (print_values)
      for (size_t i = 0; i < float_arr.len; ++i)
        os << i << '.' << ' ' << float_arr.vals[i] << '\n';
  }

  else if (arr.variant() == Array::VARIANT_STRING) {
    auto const& str_arr = arr.str_arr();
    os << "str column len=" << str_arr.len
       << " width=" << str_arr.width << '\n';
    if (print_values)
      for (size_t i = 0; i < str_arr.len; ++i) {
        os << i << '.' << ' ' << '[';
        char const* base = str_arr.chars.data();
        for (char const* p = base + i * str_arr.width;
             p < base + (i + 1) * str_arr.width;
             ++p)
          if (*p == 0)
            os << "·";
          else
            os << *p;
        os << ']' << '\n';
      }
  }
}


inline std::ostream&
operator<<(
  std::ostream& os,
  Array const& arr)
{
  print(os, arr, false);
  return os;
}


Array
parse_array(
  Column const& col,
  bool header=true)
{
  {
    auto arr = parse_number_arr<int64_t>(col);
    if (arr)
      return {std::move(*arr)};
  }
  {
    auto arr = parse_number_arr<float64_t>(col);
    if (arr)
      return {std::move(*arr)};
  }
  
  return {parse_str_arr(col)};
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
    auto arr = parse_array(col, true);
    std::cout << arr << "\n";
  }

  // FIXME: munmap.
  // FIXME: close.

  return 0;
}


