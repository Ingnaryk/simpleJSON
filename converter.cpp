#include "converter.h"

#include "type.h"

Converter::Read Converter::convert_read(char c) {
  switch (c) {
    case 'x':
      return Read::x;
    case 'u':
      return Read::u;
    case 'U':
      return Read::U;
    default:
      if ('0' <= c && c <= '7')
        return Read::_0to7;
      else if (c == '8' || c == '9')
        return Read::_8to9;
      else if ('a' <= c && c <= 'f' || 'A' <= c && c <= 'F')
        return Read::atof_AtoF;
      return Read::other;
  }
}

std::optional<std::string> Converter::deal(char c) {
  state = state_table[(int)state][(int)convert_read(c)];
  switch (state) {
    case State::octal:
      ++count;
      value = value * 8 + c - '0';
      break;
    case State::hex:
    case State::ucs2:
    case State::ucs4:
      if (c != 'x' && c != 'u' && c != 'U') {
        ++count;
        value = value * 16 +
                (c > '9' ? (c > 'F' ? c - 'a' : c - 'A') + 10 : c - '0');
      }
      break;
    default:
      break;
  }
  LOG("converter read: c=%c,count=%d,value=%u,state=%d", c, count, value,
      (int)state);
  if (state == State::end)
    return std::string(1, c);
  else if (count < counts[(int)state])
    return std::nullopt;
  LOG("converter returns value");
  switch (state) {
    case State::octal:
    case State::hex:
      return std::string(1, static_cast<char>(value));
    case State::ucs2:
      return std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t>{}
          .to_bytes(static_cast<char16_t>(value));
    case State::ucs4:
      return std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t>{}
          .to_bytes(value);
    default:
      break;
  }
  return std::nullopt;
}