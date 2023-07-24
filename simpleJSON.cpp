#include "simpleJSON.h"

#include "converter.h"

char basic_escape(char c) {
  // [note]: compatible with \a and \v which are unsupported by JSON, reserve \0
  // for octal escape
  switch (c) {
    case 'a':
      return '\a';
    case 'b':
      return '\b';
    case 'f':
      return '\f';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    case 'v':
      return '\v';
    default:
      return c;
  }
}

std::string anti_escape(char c) {
  switch (c) {
    case '\a':
      return "\\a";
    case '\b':
      return "\\b";
    case '\f':
      return "\\f";
    case '\n':
      return "\\n";
    case '\r':
      return "\\r";
    case '\t':
      return "\\t";
    case '\v':
      return "\\v";
    default:
      return std::string{1, c};
  }
}

/*
 * [note]: string to number functions comparation
 * atoi/atof: C function, accept only <const char *>, never returns an error,
 * hard to judge if error occurs according to the converted value
 *
 * strtol/strtod: C function, accepts additional <char **> param indicating the
 * last resolved address to help judge error
 *
 * std::stoi/std::stod: c++11 function, accepts <std::string> as well as <size_t
 * &> which indicates the resolved characters' length
 *
 * std::from_chars: c++17 overloaded function, supports more number types and
 * other options, returns a struct consisting of <const char *>, the last
 * resolved address, and <std::errc> explicitly indicating what type of error
 * occurs
 *
 * other methods: sscanf, std::istringstream, need a buffer or temp variable,
 * and have performance loss
 */
template <typename T>
std::optional<T> try_parse_num(
    std::string
        str) {  // [note]: use std::string here instead of std::string_view
                // because we assume that a number string won't be too long and
                // std::string has a small sequence optimization
  static_assert(std::is_same_v<T, int> || std::is_same_v<T, double>);
  size_t end_loc{};
  T value;
  if constexpr (std::is_same_v<T, int>)
    value = std::stoi(str, &end_loc);
  else if constexpr (std::is_same_v<T, double>)
    value = std::stod(str, &end_loc);
  if (end_loc != str.size()) return std::nullopt;
  return value;
}

/*
 * strict JSON number: -?(0|[1-9]+[0-9]*)(\.[0-9]*)?([eE][+-]?[0-9]+)?
 * compatible with '+', '.' and zeros prefix
 * so the regex is [+-]?[0-9]+(\.[0-9]*)?([eE][+-]?[0-9]+)?
 */
std::pair<JSONObject, size_t> parse_detail(std::string_view json) {
  static constexpr std::string_view spaces = " \f\n\r\t\v";
  if (json.empty()) {
    return {JSONObject{std::monostate{}}, 0};
  } else if (size_t prefix = json.find_first_not_of(spaces);
             prefix != 0 && prefix != std::string_view::npos) {
    auto [obj, eaten] = parse_detail(json.substr(prefix));
    return {
        JSONObject{std::move(obj)},
        eaten + prefix};  // [note]: unified management of empty prefixes, no
                          // need to compensate prefix in every return statement
  } else if (json.substr(0, 4) == "null") {
    return {JSONObject{std::monostate{}}, 4};
  } else if (json.substr(0, 4) == "true") {
    return {JSONObject{true}, 4};
  } else if (json.substr(0, 5) == "false") {
    return {JSONObject{false}, 5};
  } else if (json.find_first_of("0123456789+-.") == 0) {
    static thread_local const std::regex num_re{
        R"([+-]?[0-9]+(\.[0-9]*)?([eE][+-]?[0-9]+)?)"};
    std::cmatch match;
    if (std::regex_search(json.begin(), json.end(), match, num_re)) {
      std::string str = match.str();
      if (auto num = try_parse_num<int>(str)) {
        return {JSONObject{*num}, str.size()};
      }
      if (auto num = try_parse_num<double>(str)) {
        return {JSONObject{*num}, str.size()};
      }
    }
  } else if (char quote = json[0]; quote == '"' || quote == '\'') {
    std::string str;
    std::string_view::iterator itor;
    enum { Raw, Escaped, Codecvt } phase = Raw;
    char hex_value{};
    for (itor = json.begin() + 1; itor != json.end(); ++itor) {
      char ch = *itor;
      if (phase == Raw) {
        if (ch == '\\')
          phase = Escaped;
        else if (ch == quote)
          break;
        else
          str += ch;
      } else if (phase == Escaped) {
        if (char escaped = basic_escape(ch); escaped != ch) {
          str += escaped;
          phase = Raw;
        } else {
          --itor;
          phase = Codecvt;
        }
      } else if (phase == Codecvt) {
        static thread_local Converter cvt;
        if (auto result = cvt.deal(ch)) {
          str += *result;
          phase = Raw;
          cvt.reset();
        }
      }
    }
    return {JSONObject{std::move(str)}, itor - json.begin() + 1};
  } else if (json[0] == '[') {
    JSONList list;
    size_t whole_eaten = 1;
    for (; whole_eaten < json.size();) {
      if (json[whole_eaten] == ']') {
        whole_eaten += 1;
        break;
      }
      auto [obj, eaten] = parse_detail(json.substr(whole_eaten));
      if (eaten == 0) {
        whole_eaten = 0;
        break;
      }
      list.emplace_back(obj);
      whole_eaten += eaten;
      while (spaces.find(json[whole_eaten]) != std::string_view::npos)
        ++whole_eaten;
      if (json[whole_eaten] == ',')
        whole_eaten += 1;
      else if (json[whole_eaten] != ']') {
        whole_eaten = 0;
        break;
      }
    }
    LOG("list eaten: %u", whole_eaten);
    return {JSONObject{std::move(list)}, whole_eaten};
  } else if (json[0] == '{') {
    JSONDict dict;
    size_t whole_eaten = 1;
    for (; whole_eaten < json.size();) {
      if (json[whole_eaten] == '}') {
        whole_eaten += 1;
        break;
      }
      auto [key, k_eaten] = parse_detail(json.substr(whole_eaten));
      if (k_eaten == 0) {
        whole_eaten = 0;
        break;
      }
      whole_eaten += k_eaten;
      while (spaces.find(json[whole_eaten]) != std::string_view::npos)
        ++whole_eaten;
      if (json[whole_eaten] == ':')
        whole_eaten += 1;
      else {
        whole_eaten = 0;
        break;
      }
      auto [value, v_eaten] = parse_detail(json.substr(whole_eaten));
      if (v_eaten == 0) {
        whole_eaten = 0;
        break;
      }
      dict.insert_or_assign(key, value);
      whole_eaten += v_eaten;
      while (spaces.find(json[whole_eaten]) != std::string_view::npos)
        ++whole_eaten;
      if (json[whole_eaten] == ',')
        whole_eaten += 1;
      else if (json[whole_eaten] != ']') {
        whole_eaten = 0;
        break;
      }
    }
    LOG("dict eaten: %u", whole_eaten);
    return {JSONObject{std::move(dict)}, whole_eaten};
  }
  return {JSONObject{std::monostate{}}, 0};
}

JSONObject parse(std::string_view json) {
  JSONObject result;
  std::tie(result, std::ignore) = parse_detail(json);
  return result;
}

std::ostream &operator<<(std::ostream &os, const JSONObject &obj) {
  // [note]: in c++23 we won't need to explicitly add a 'self' argument for
  // recursive call
  auto formatter = [&](auto &self, const JSONObject &obj,
                       bool as_key = false) -> void {
    std::visit(overloaded{[&](std::monostate) { os << "null"; },
                          [&](bool b) { os << std::boolalpha << b; },
                          [&](int i) { os << i; }, [&](double d) { os << d; },
                          [&](const std::string &s) {
                            if (as_key)
                              std::transform(
                                  s.begin(), s.end(),
                                  std::ostream_iterator<std::string>(os),
                                  anti_escape);
                            else
                              os << std::quoted(s);
                          },
                          [&](const JSONList &list) {
                            os << "[";
                            if (!as_key) {
                              size_t loc{};
                              for (const auto &element : list) {
                                self(self, element);
                                os << (++loc == list.size() ? "" : ", ");
                              }
                            } else if (!list.empty()) {
                              os << "...";
                            }
                            os << "]";
                          },
                          [&](const JSONDict &dict) {
                            os << "{";
                            if (!as_key) {
                              size_t loc{};
                              for (const auto &[key, value] : dict) {
                                self(self, key, true);
                                os << ": ";
                                self(self, value);
                                os << (++loc == dict.size() ? "" : ", ");
                              }
                            } else if (!dict.empty()) {
                              os << "...";
                            }
                            os << "}";
                          }},
               obj.inner);
  };
  formatter(formatter, obj);
  return os;
}