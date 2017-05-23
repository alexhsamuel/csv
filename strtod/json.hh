#pragma once

#include <cassert>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

using namespace std::string_literals;

namespace aslib {
namespace json {

//------------------------------------------------------------------------------

extern int const FORMAT_MIN;
extern int const FORMAT_ONE_LINE;

//------------------------------------------------------------------------------

class Error
  : std::exception
{
public:

  Error(std::string const& what) : what_(what) {}
  ~Error() throw() override {}
  char const* what() const throw() override { return what_.c_str(); }

private:

  std::string const what_;

};


class TypeError
  : Error
{
public:

  using Error::Error;
  virtual ~TypeError() throw() override {}

};


// FIXME: Provide more information.
class ParseError
  : Error
{
public:

  ParseError() : Error("parse error") {}
  virtual ~ParseError() throw() override {}

};


class NameError
  : Error
{
public:

  NameError(std::string const& name) : Error("name error: "s + name) {}
  virtual ~NameError() throw() override {}

};


class IndexError
  : Error
{
public:

  IndexError(
    size_t const index, 
    size_t const size)
  : Error(what(index, size))
  {
  }

  virtual ~IndexError() throw() override {}

private:

  static std::string 
  what(
    size_t const index,
    size_t const size)
  {
    std::ostringstream ss;
    ss << "index error: index " << index << " for size " << size;
    return ss.str();
  }

};


//------------------------------------------------------------------------------

class Json
{
public:

  enum Type
  {
    NUL,
    FAL,
    TRU,
    NUM,
    STR,
    ARR,
    OBJ,
  };

  typedef std::vector<Json> ArrVal;
  typedef std::map<std::string, Json> ObjVal;

  // Ctors.
  Json()                                { set(NUL); }
  Json(Type type)                       { set(type); }
  Json(double val)                      { set(val); }
  Json(std::string const& val)          { set(val); }
  Json(Json const& json)                = delete;
  Json(Json&& json)                     { move(json); }
  // FIXME: Add deep copy ctor?

  static Json new_arr()                 { return Json(ARR); }
  static Json new_obj()                 { return Json(OBJ); }

  // Convenience ctors.
  Json(bool val)                        { set(val ? TRU : FAL); }
  Json(int val)                         { set((double) val); }
  Json(char const* val)                 { set(std::string(val)); }

  virtual ~Json()                       { clear(); }

  Json& operator=(Json const& json)     = delete;
  Json& operator=(Json&& json)          { clear(); move(json); return *this; }

  Type get_type() const                 { return type_; }

  // Accessors for FAL and TRU.
  bool get_bool() const;

  // Accessors for NUM.
  double get_num() const;

  // Accessors for STR;
  std::string const& get_str() const;

  // Accessors for ARR;
  size_t size() const                   { return get_arr().size(); }
  Json const& operator[](size_t index) const;
  Json& operator[](size_t index);
  ArrVal const& get_arr() const;

  // Accessors for OBJ.
  bool has(std::string const& name) const;
  Json const& operator[](std::string const& name) const;
  Json& operator[](std::string const& name);
  ObjVal const& get_obj() const;

  // Convenience accessors.
  int get_int() const;

  virtual void print(std::ostream& os, int indent, size_t level=0) const;

private:

  void set(Type type);
  void set(double val);
  void set(std::string const& val);
  void move(Json& json);

  double& get_num();
  std::string& get_str();
  ArrVal& get_arr();
  ObjVal& get_obj();

  void clear();

  Type type_;
  void* val_;  // FIXME: Use a better representation.

};


inline bool
Json::get_bool()
  const
{
  switch (type_) {
  case FAL: return false;
  case TRU: return true;

  default:
    throw TypeError("not a TRU or FAL");
  }
}


inline double 
Json::get_num()
  const
{
  if (type_ == NUM)
    return *reinterpret_cast<double const*>(val_);
  else
    throw TypeError("not a NUM");
}


inline std::string const&
Json::get_str()
  const
{
  if (type_ == STR) 
    return *reinterpret_cast<std::string const*>(val_);
  else
    throw TypeError("not a STR");
}


inline Json const&
Json::operator[](
  size_t index)
  const
{
  ArrVal const& arr = get_arr();
  if (index < arr.size())
    return arr[index];
  else
    throw IndexError(index, arr.size());
}


inline Json&
Json::operator[](
  size_t index)
{
  ArrVal& arr = get_arr();
  if (index < arr.size())
    return arr[index];
  else if (index == arr.size()) {
    // Append a new NUL.
    arr.emplace_back();
    // Return a reference to it.
    return arr[index];
  }
  else
    throw IndexError(index, arr.size());
}


inline bool
Json::has(
  std::string const& name)
  const
{
  ObjVal const& obj = get_obj();
  auto find = obj.find(name);
  return find != end(obj);
}


inline Json const&
Json::operator[](
  std::string const& name)
  const
{
  ObjVal const& obj = get_obj();
  auto find = obj.find(name);
  if (find == obj.end())
    throw NameError(name);
  else
    return find->second;
}


inline Json&
Json::operator[](
  std::string const& name)
{
  ObjVal& obj = get_obj();
  auto find = obj.find(name);
  if (find == obj.end()) 
    // Name not found.  Set it to a new NUL, and return a reference to it.
    return obj[name];
  else
    return find->second;
}


inline int
Json::get_int()
  const
{
  double const num = get_num();
  int const val = int(num);
  if (val == num)
    return val;
  else {
    std::stringstream ss;
    ss << "not an int: " << val;
    throw TypeError(ss.str());
  }
}


inline void
Json::set(
  Type type)
{
  switch (type) {
  case NUL:
  case TRU:
  case FAL:
    type_ = type;
    break;

  case NUM:
  case STR:
    assert(false);

  case ARR:
    type_ = ARR;
    val_ = new ArrVal;
    break;

  case OBJ:
    type_ = OBJ;
    val_ = new ObjVal;
    break;
  }
}


inline void
Json::set(
  double val)
{
  type_ = NUM;
  val_ = new double(val);
}


inline void
Json::set(
  std::string const& val)
{
  type_ = STR;
  val_ = new std::string(val);
}


inline void
Json::move(
  Json& json)
{
  type_ = json.type_;
  val_ = json.val_;
  json.type_ = NUL;
}


inline Json::ArrVal const&
Json::get_arr()
  const
{
  if (type_ == ARR)
    return *reinterpret_cast<ArrVal const*>(val_);
  else
    throw TypeError("not a ARR");
}


inline Json::ObjVal const&
Json::get_obj()
  const
{
  if (type_ == OBJ)
    return *reinterpret_cast<ObjVal const*>(val_);
  else
    throw TypeError("not a OBJ");
}


inline double&
Json::get_num()
{
  if (type_ == NUM)
    return *reinterpret_cast<double*>(val_);
  else
    throw TypeError("not a NUM");
}


inline std::string&
Json::get_str()
{
  if (type_ == STR)
    return *reinterpret_cast<std::string*>(val_);
  else
    throw TypeError("not a STR");
}


inline Json::ArrVal&
Json::get_arr()
{
  if (type_ == ARR)
    return *reinterpret_cast<ArrVal*>(val_);
  else
    throw TypeError("not a ARR");
}


inline Json::ObjVal&
Json::get_obj()
{
  assert(type_ == OBJ);
  return *reinterpret_cast<ObjVal*>(val_);
}


inline void
Json::clear()
{
  switch (type_) {
  case NUL:
  case FAL:
  case TRU:
    break;

  case NUM:
    delete & get_num();
    break;

  case STR:
    delete & get_str();
    break;

  case ARR:
    delete & get_arr();
    break;
    
  case OBJ:
    delete & get_obj();
    break;
  }

  type_ = NUL;
}


//------------------------------------------------------------------------------

inline std::ostream&
operator<<(
  std::ostream& os,
  Json const& json)
{
  json.print(os, 2);
  return os;
}

extern Json parse(std::string const& json);
extern Json parse(std::string const& json, std::string::size_type& end);

//------------------------------------------------------------------------------

class Serializable
{
public:

  virtual ~Serializable() {}
  virtual Json to_json() const = 0;

};


//------------------------------------------------------------------------------

}  // namespace json
}  // namespace aslib

