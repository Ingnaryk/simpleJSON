#pragma once

#include <charconv>
#include <iomanip>
#include <iterator>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "type.h"

struct JSONObject;

// [note]: forward declaration of JSONObject's hash function to avoid imcomplete
// type, as JSONObject definition itself relying on JSONDict's hash function
struct JSONObjectHash {
  inline size_t operator()(const JSONObject &obj) const;
};

using JSONList = std::vector<JSONObject>;
using JSONDict = std::unordered_map<JSONObject, JSONObject, JSONObjectHash>;

struct JSONObject {
  std::variant<std::monostate,  // null
               bool,            // true/false
               int,             // 42
               double,          // 3.14
               std::string,     // "hello"
               JSONList,        // [42, "hello"]
               JSONDict         // {"hello": 985, "world": 211}
               >
      inner;  // [note]: use struct wrapping std::variant to enable self nested
              // in std::variant declaration

  bool operator==(const JSONObject &other) const {
    return this->inner == other.inner;  // [note]: use std::variant::operator==
  }
};

size_t JSONObjectHash::operator()(const JSONObject &obj) const {
  // [note]: cannot use std::hash<std::variant> unless each element of
  // std::variant can be hashed, that is to say, we need to provide JSONList and
  // JSONDict's hash function
  return std::visit(overloaded{
                        [](const JSONList &list) {
                          return std::hash<const JSONList *>{}(&list);
                        },
                        [](const JSONDict &dict) {
                          return std::hash<const JSONDict *>{}(&dict);
                        },
                        [](auto basic) -> size_t {
                          return std::hash<decltype(basic)>{}(basic);
                        },
                    },
                    obj.inner);
}

std::ostream &operator<<(std::ostream &os, const JSONObject &obj);

template <typename T>
std::optional<T> try_parse_num(std::string str);

std::pair<JSONObject, size_t> parse_detail(std::string_view json);

JSONObject parse(std::string_view json);