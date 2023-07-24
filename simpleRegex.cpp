#include <iostream>
#include <string_view>
#include <vector>

struct cmatch {
  using pstr = const char *;
  using sub_match = std::pair<pstr, pstr>;
  std::vector<sub_match> matches;
};

// . * ?
bool is_match(std::string_view str, std::string &pattern) {
#define index(i) (i - 1)
#define matches(i, j) \
  (i != 0 && (str[index(i)] == pattern[index(j)] || pattern[index(j)] == '.'))
  const int m = str.size(), n = pattern.size();
  std::vector<std::vector<bool>> dp(m + 1, std::vector<bool>(n + 1, false));
  dp[0][0] = true;
  for (int i = 0; i <= m; ++i) {
    for (int j = 1; j <= n; ++j) {
      if (pattern[index(j)] != '*') {
        dp[i][j] = matches(i, j) && dp[i - 1][j - 1];
      } else {
        dp[i][j] = dp[i][j - 2] || (matches(i, j - 1) && dp[i - 1][j]);
      }
    }
  }
  return dp[m][n];
#undef matches
#undef index
}

bool regex_search(std::string_view str, cmatch &match, std::string pattern) {}

int main(int argc, char *argv[]) {
  std::string str, pattern;
  if (argc < 2)
    std::cin >> str >> pattern;
  else if (argc < 3) {
    str = argv[1];
    std::cin >> pattern;
  } else {
    str = argv[1];
    pattern = argv[2];
  }
  std::cout << std::boolalpha << is_match(str, pattern) << '\n';
}