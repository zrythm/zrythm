#pragma once

#include <string>
#include <vector>

#include "Converter.hpp"

class ZshConverter : public Converter {
  private:
    std::ofstream file;
  public:
    ZshConverter() {
        file.open("zsh-converter.out", std::ios::trunc);
        file.close();
        file.open("zsh-converter.out", std::ios::out | std::ios::app);
    };
    ~ZshConverter() {
        file.close();
    };
    void convert(std::string &command, std::string &description,
                         std::vector<std::string> &short_option,
                         std::vector<std::string> &long_option,
                         std::vector<std::string> &old_option);
};
