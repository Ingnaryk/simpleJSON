#pragma once

#include <codecvt>
#include <locale>
#include <optional>
#include <string>

/*
        x       u       U       0-7     8-9     a-f/A-F     other
start   hex     ucs2    ucs4    octal   end     end         end
octal   end     end     end     octal   end     end         end
hex     end     end     end     hex     hex     hex         end
ucs2    end     end     end     ucs2    ucs2    ucs2        end
ucs4    end     end     end     ucs4    ucs4    ucs4        end
end     end     end     end     end     end     end         end
*/

class Converter {
 private:
  enum class State { start, octal, hex, ucs2, ucs4, end, LENGTH };
  enum class Read { x, u, U, _0to7, _8to9, atof_AtoF, other, LENGTH };
  static constexpr State state_table[(int)State::LENGTH][(int)Read::LENGTH] = {
      {State::hex, State::ucs2, State::ucs4, State::octal, State::end,
       State::end, State::end},
      {State::end, State::end, State::end, State::octal, State::end, State::end,
       State::end},
      {State::end, State::end, State::end, State::hex, State::hex, State::hex,
       State::end},
      {State::end, State::end, State::end, State::ucs2, State::ucs2,
       State::ucs2, State::end},
      {State::end, State::end, State::end, State::ucs4, State::ucs4,
       State::ucs4, State::end},
      {State::end, State::end, State::end, State::end, State::end, State::end,
       State::end},
  };
  static constexpr int counts[(int)State::LENGTH] = {0, 3, 2, 4, 8, 0};
  Read convert_read(char c);
  int count = 0;
  char32_t value = 0;
  State state = State::start;

 public:
  std::optional<std::string> deal(char c);
  void reset() {
    count = value = 0;
    state = State::start;
  }
};