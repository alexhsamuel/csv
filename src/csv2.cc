#include <cassert>
#include <cstdlib>
#include <cstring>

#include "strtod/parse_double.h"
#include "csv2.hh"

// FIXME: UTF-8 (and other encodings?)

std::vector<Column>
split_columns(
  Buffer const buffer,
  char const sep,
  char const eol,
  char const quote)
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

StrArr
parse_str_arr(
  Column const& col,
  bool const header=true)
{
  auto const len = col.size();
  if (len == 0)
    return StrArr{0, 0, {}};

  // Compute the column width, which is the longest string length.
  auto const width = col.max_width();
  std::vector<char> chars(len * width, 0);
  char* base = chars.data();

  size_t i = 0;
  auto fields = col.begin();

  std::string name;
  if (header) {
    auto const& n = *fields;
    name = std::string{n.ptr, n.len};
    ++fields;
  }

  for (; fields != col.end(); ++fields) {
    auto const field = *fields;
    memcpy(base + i++ * width, field.ptr, field.len);
  }

  return {len, width, std::move(chars), name};
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

  auto const val = parse_double_6(buf.ptr, buf.ptr + buf.len);
  if (is_parse_error(val))
    return {};
  else
    return val;
}


//------------------------------------------------------------------------------

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


Array
parse_array(
  Column const* col,
  bool header)
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

void
print(
  std::ostream& os,
  Array const& arr,
  bool const print_values)
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


