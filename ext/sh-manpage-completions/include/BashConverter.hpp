#pragma once

#include <iomanip>
#include <string>
#include <vector>

#include "Converter.hpp"

class BashConverter : public Converter {
  private:
    std::ofstream file, descriptionsFile;
    std::vector<std::vector<std::string> > descriptions;
    size_t largest_joined_options_length;

  public:
    BashConverter() {
        largest_joined_options_length = 0;
        file.open("bash-converter.out", std::ios::trunc);
        file.close();
        file.open("bash-converter.out", std::ios::trunc);

        descriptionsFile.open("bash-converter-descriptions.out",
                              std::ios::trunc);
        descriptionsFile.close();
        descriptionsFile.open("bash-converter-descriptions.out",
                              std::ios::out | std::ios::app);
    };
    ~BashConverter() {
        file.close();
        for (auto &it : descriptions) {
            // Right-align options
            // descriptionsFile << std::setw(largest_joined_options_length);
            descriptionsFile << it[0] << ": " << it[1] << std::endl;
        }
        descriptionsFile.close();
    };
    void convert(std::string &command, std::string &description,
                 std::vector<std::string> &short_option,
                 std::vector<std::string> &long_option,
                 std::vector<std::string> &old_option);
};
