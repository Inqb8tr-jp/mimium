#pragma once

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace mimium {
// https://medium.com/@dennis.luxen/breaking-circular-dependencies-in-recursive-union-types-with-c-17-the-curious-case-of-4ab00cfda10d
template <typename T>
struct recursive_wrapper {
  // construct from an existing object
  recursive_wrapper(T t_) { t.emplace_back(std::move(t_)); }  // NOLINT
  // cast back to wrapped type
  operator const T&() const { return t.front(); }  // NOLINT
  // store the value
  std::vector<T> t;
  std::string toString() { return t.front().toString(); };
};

namespace types {
struct Void {
  std::string toString() { return "Void"; }
};
struct Float {
  std::string toString() { return "Float"; };
};
struct String {
  std::string toString() { return "String"; };
};

struct Function;
struct Array;
struct Struct;
struct Time;
using Value = std::variant<types::Void, types::Float, types::String,
                           recursive_wrapper<types::Function>,
                           recursive_wrapper<types::Array>,
                           recursive_wrapper<types::Struct>, recursive_wrapper<types::Time>>;
struct Time {
  Value val;
  Float time;
  Time() = default;
  std::string toString() { return std::visit([](auto c) { return c.toString(); }, val); + "@" + time.toString(); }
};
struct Function {
  Function() = default;
  Function(std::vector<Value> arg_types_p, Value ret_type_p)
      : arg_types(std::move(arg_types_p)), ret_type(std::move(ret_type_p)){};
  void init(std::vector<Value> arg_types_p, Value ret_type_p) {
    arg_types = std::move(arg_types_p);
    ret_type = std::move(ret_type_p);
  }
  std::vector<Value> arg_types;
  Value ret_type;
  Value& getReturnType() { return ret_type; }
  std::vector<Value>& getArgTypes() { return arg_types; }
  std::string toString() {
    std::string s = "Fn[ (";
    int count = 1;
    for (const auto& v : arg_types) {
      s += std::visit([](auto c) { return c.toString(); }, v);
      if (count < arg_types.size()) s += " , ";
      count++;
    }
    s += ") -> " + std::visit([](auto c) { return c.toString(); }, ret_type) +
         " ]";
    return s;
  };
  template <class... Args>
  static std::vector<Value> createArgs(Args&&... args) {
    std::vector<Value> a = {std::forward<Args>(args)...};
    return a;
  }
};
struct Array {
  Value elem_type;
  explicit Array(Value elem) : elem_type(std::move(elem)) {}
  std::string toString() {
    return "array[" +
           std::visit([](auto c) { return c.toString(); }, elem_type) + "]";
  }
  Value getElemType() { return elem_type; }
};
struct Struct {
  std::vector<Value> arg_types;
  explicit Struct(std::vector<Value> types) : arg_types(std::move(types)) {}
  std::string toString() {
    std::string s;
    s += "struct{";
    for (auto& a : arg_types) {
      s += std::visit([](auto c) { return c.toString(); }, a) + " ";
    }
    s += "}";
    return s;
  }
};

}  // namespace types
class TypeEnv {
 public:
  std::unordered_map<std::string, types::Value> env;
  std::string dump() {
    std::stringstream ss;
    for (auto& it : env) {
      ss << it.first << " : "
         << std::visit([](auto t) { return t.toString(); }, it.second)
         << std::endl;
    }
    return ss.str();
  }
};
}  // namespace mimium