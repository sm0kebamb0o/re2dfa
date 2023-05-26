#include "api.hpp"
#include <iostream>
#include <fstream>
#include <string>

extern DFA re2dfa(const std::string& regExp);

int main() {
  std::ifstream infile("in.txt");
  std::ofstream outfile("out.txt");

  std::string line;
  std::getline(infile, line);

  line = "a|babaabba|";

  std::cout << "-----------";
  std::cout << line << '\n';
  std::cout << "-----------";

  std::cout << re2dfa(line).to_string();

  //outfile << re2dfa(line).to_string();

  return 0;
}
