#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

class Converter {
  public:
    virtual void convert(std::string &command, std::string &description,
                         std::vector<std::string> &short_option,
                         std::vector<std::string> &long_option,
                         std::vector<std::string> &old_option) = 0;
};
