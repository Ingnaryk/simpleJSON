#include <iostream>

#include "simpleJSON.h"

int main() {
  /*** basic test ***/
  // [note]: providing parse_detail interface to check if a parse is valid or
  // not in JSON parse an empty string causes an error in this interface
  // returned value is null but with 0 characters eaten compared with exactly
  // parsing "null" returning 4 characters eaten
  auto [result, eaten] = parse_detail("");
  std::cout << "parse_detail(\"\") returns result=" << result
            << ", eaten=" << eaten << '\n';
  // support prefix spaces
  for (auto bs : {" true", "\tfalse"}) {
    JSONObject result = parse(bs);
    std::cout << std::get<bool>(result.inner) << '\n';
  }
  // JSON number's integral part cannot start with '+', '.' or zeros
  // to improve robustness, we support it
  for (auto is : {"+72", "02", "-0625"}) {
    JSONObject result = parse(is);
    std::cout << std::get<int>(result.inner) << '\n';
  }
  // all the numbers with e/E are of floating point
  // and in JSON only the exponent part can start with zeros
  for (auto ds : {"7.2", "-2.5e3", "2.573e02"}) {
    JSONObject result = parse(ds);
    std::cout << std::get<double>(result.inner) << '\n';
  }
  for (auto ss :
       {R"("string")", R"("\"escaped\"\n")", R"("bad escaped format \g")"}) {
    JSONObject result = parse(ss);
    std::cout << std::get<std::string>(result.inner) << '\n';
  }
  /*** test operator<< overload, nested structure ***/
  // test \t escape
  JSONObject list = parse(
      "[null, false, 42.2, [985, 211, {}], \"a long long long\tstring\"]");
  std::cout << list << '\n';
  // test \xhh \ddd escape
  JSONObject dict = parse(
      R"({"hello\x0aworld" : "JSON", 985: 121, 'nested\012':
{"array": [1 , 2 , 3]}})");
  std::cout << dict << '\n';
  // test \uhhhh, \Uhhhhhhhh escape, to display Chinese on windows, need chcp
  // 65001
  std::cout << parse(R"({
"Alice\u000a": "\u6211\u7231\u4f60!",
"Steve\U0000000a": "\U00004f60\U00007231\U00006211?"
})") << '\n';
  // test JSONObject as JSONDict' s key
  JSONObject wrap_dict{
      JSONDict{{{"introduction"}, {"use any JSONObject as dict's key"}},
               {list, {"list"}},
               {dict, {"dict"}},
               {{JSONList{}}, {"empty list"}},
               {{JSONDict{}}, {"empty dict"}}}};
  std::cout << wrap_dict << '\n';
  /*** test bad formats compatibility ***/
  std::cout << "bad format1: " << parse("[1, 2,]") << '\n';   // [1, 2]
  std::cout << "bad format2: " << parse("[1, 2") << '\n';     // [1, 2]
  std::cout << "bad format3: " << parse("[1, , , ") << '\n';  // [1, null, null]
  std::cout << "bad format4: " << parse("{'k\"ey':  [1, , ")
            << '\n';  // {k"ey: [1, null]}
  std::cout << "bad format5: " << parse(R"({'key'  [1, , )") << '\n';     // {}
  std::cout << "bad format6: " << parse(R"({'bad key'':  6} )") << '\n';  // {}
  std::cout << "bad format7: " << parse(R"([7, 3..14])") << '\n';     // [7, 3]
  std::cout << "bad format8: " << parse(R"([8, 8b, 12.7])") << '\n';  // [8, 8]
  std::cout << "bad format9: " << parse(R"([9, 5.7e3.6, 0])")
            << '\n';  // [9, 5700]
}