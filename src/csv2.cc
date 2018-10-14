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

#include "strtod/parse_double.h"
#include "ThreadPool.hh"

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

class Column
{
private:

  // FIXME: Use a more efficient encoding.
  struct Field
  {
    Field() : missing{true} {};

    Field(
      size_t start_,
      size_t len_,
      bool escaped_) 
    : start{start_}
    , len{len_}
    , escaped{escaped_}
    , missing{false} 
    {
    }

    unsigned long   start   : 46;  // 64 TB limit on total size
    unsigned long   len     : 16;  // 64 KB limit on field size
    bool            escaped :  1;
    bool            missing :  1;
  };

  static_assert(sizeof(Field) == 8, "wrong sizeof(Field)");

public:

  Column(
    Buffer const buf, 
    size_t const num_missing=0)
  : buf_(buf)
  , fields_(num_missing, Field{})
  , has_missing_(num_missing > 0)
  {
  }

  Column(Column&&) = default;

  size_t    size()          const { return fields_.size(); }
  bool      has_missing()   const { return has_missing_; }
  bool      has_empty()     const { return has_empty_; }
  size_t    max_width()     const { return max_width_; }

  void append(
    size_t const start,
    size_t const end,
    bool const escaped)
  {
    assert(0 <= start && start < buf_.len);
    assert(start <= end && end <= buf_.len);

    auto const len = end - start;
    fields_.emplace_back(start, len, escaped);

    if (len == 0)
      has_empty_ = true;
    if (len > max_width_)
      max_width_ = len;
  }

  void 
  append_missing()
  {
    fields_.emplace_back();
    has_missing_ = true;
  }

  class Iterator
  {
  public:

    Iterator(
      Column const& col, 
      size_t idx=0) 
    : col_(col),
      idx_(idx)
    {
      assert(0 <= idx_ && idx_ <= col_.size());
    }

    bool operator==(Iterator const& it) const { return it.idx_ == idx_; }
    bool operator!=(Iterator const& it) const { return it.idx_ != idx_; }
    void operator++() { ++idx_; }

    Buffer
    operator*() 
      const
    {
      auto const& field = col_.fields_.at(idx_);
      if (field.missing)
        return {nullptr, 0};
      else
        return {col_.buf_.ptr + field.start, field.len};
    }

  private:

    Column const& col_;
    size_t idx_;

  };

  Iterator begin() const { return Iterator(*this, 0); }
  Iterator end() const { return Iterator(*this, size()); }

private:

  Buffer const buf_;
  std::vector<Field> fields_;
  bool has_missing_ = false;
  bool has_empty_ = false;
  size_t max_width_ = 0;

};


std::vector<Column>
split_columns(
  Buffer const buffer,
  char const sep=',',
  char const eol='\n',
  char const quote='"')
{
  if (buffer.len == 0)
    return {};

  std::vector<Column> cols;
  // FIXME: Replace col_idx with an iterator?
  std::vector<Column>::size_type col_idx = 0;
  cols.emplace_back(buffer);

  size_t start = 0;
  bool escaped = false;

  for (size_t i = 0; i < buffer.len; ++i) {
    char const c = buffer.ptr[i];

    if (c == eol) {
      // End the field.
      cols.at(col_idx++).append(start, i, escaped);
      escaped = false;
      start = i + 1;

      // Remaining colums are missing in this row.
      for (; col_idx < cols.size(); ++col_idx)
        cols.at(col_idx).append_missing();

      // End the line.
      col_idx = 0;
    }
    else if (c == sep) {
      // End the field.
      cols.at(col_idx++).append(start, i, escaped);
      escaped = false;
      start = i + 1;

      if (col_idx >= cols.size())
        // Create a new column, and back-fill it with missing.
        cols.emplace_back(buffer, cols[0].size() - 1);
    }
    else if (c == quote) {
      // Fast-forward through quoted strings: skip over the opening quote, and
      // copy characters until the closing quote.
      escaped = true;
      for (++i; i < buffer.len && buffer.ptr[i] != quote; ++i)
        ;
      // FIXME: else: unclosed quote.
    }
    else
      // Normal character.
      // FIXME: Check for escape characters.
      ;

    // FIXME: Trailing field?
  }

  // Finish the current field.
  if (start < buffer.len)
    cols.at(col_idx++).append(start, buffer.len, escaped);

  // Remaining colums are missing in this row.
  if (col_idx > 0)
    for (; col_idx < cols.size(); ++col_idx)
      cols.at(col_idx).append_missing();

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

  size_t i = 0;
  for (auto const& field : col)
    memcpy(base + i++ * width, field.ptr, field.len);

  return {len, width, std::move(chars)};
}


//------------------------------------------------------------------------------

inline bool oadd(uint64_t a, uint64_t b, uint64_t& r) { return __builtin_uaddl_overflow(a, b, &r); }
inline bool oadd( int64_t a,  int64_t b,  int64_t& r) { return __builtin_saddl_overflow(a, b, &r); }
inline bool omul(uint64_t a, uint64_t b, uint64_t& r) { return __builtin_umull_overflow(a, b, &r); }
inline bool omul( int64_t a,  int64_t b,  int64_t& r) { return __builtin_smull_overflow(a, b, &r); }

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

  auto const val = parse_double_6(buf.ptr, buf.ptr + buf.len);
  if (is_parse_error(val))
    return {};
  else
    return val;
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

  auto fields = col.begin();

  std::string name;
  if (header) {
    auto const& n = *fields;
    name = std::string{n.ptr, n.len};
    ++fields;
  }

  // Parse the first value.
  auto const val = PARSE(*fields);
  if (!val)
    return {};
  ++fields;

  // Initialize min and max.
  T min = *val;
  T max = *val;

  // Allocate space for results.
  typename NumberArr<T>::vals_type vals;
  vals.reserve(len);
  vals.push_back(*val);

  for (; fields != col.end(); ++fields) {
    auto const& field = *fields;
    auto const val = PARSE(field);
    if (val) {
      vals.push_back(*val);
      if (*val < min)
        min = *val;
      if (*val > max)
        max = *val;
    }
    else {
      std::cerr << "FAIL: [" << field << "]\n";
      return {};
    }
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
  bool const print_values=true)
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
  Column const* col,
  bool header=true)
{
  {
    auto arr = parse_number_arr<int64_t>(*col);
    if (arr)
      return {std::move(*arr)};
  }
  {
    auto arr = parse_number_arr<float64_t>(*col);
    if (arr)
      return {std::move(*arr)};
  }
  
  return {parse_str_arr(*col)};
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
    ThreadPool pool(9);
    std::vector<std::future<Array>> results;
    // FIXME: Use std::transform.
    for (auto const& col : cols)
      results.push_back(pool.enqueue(parse_array, &col, true));

    for (auto&& result : results)
      std::cout << result.get();
  }

  // FIXME: munmap.
  // FIXME: close.

  return 0;
}


