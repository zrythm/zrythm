#include "BashConverter.hpp"

void replaceAll(std::string &source, const std::string &from,
                const std::string &to) {
    std::string newString;
    newString.reserve(source.length());

    std::string::size_type lastPos = 0;
    std::string::size_type findPos;
    while (std::string::npos != (findPos = source.find(from, lastPos))) {
        newString.append(source, lastPos, findPos - lastPos);
        newString += to;
        lastPos = findPos + from.length();
    }
    newString += source.substr(lastPos);

    source.swap(newString);
}

void BashConverter::convert(std::string &command, std::string &description,
                            std::vector<std::string> &short_option,
                            std::vector<std::string> &long_option,
                            std::vector<std::string> &old_option) {
    int num_options =
        short_option.size() + long_option.size() + old_option.size();
    if (num_options == 0) {
        description = "";
        short_option.clear();
        long_option.clear();
        old_option.clear();

        return;
    }

    std::string joined_options;
    for (auto &it : short_option) {
        joined_options += "-" + it + ",";
        file << "-" << it << std::endl;
    }
    for (auto &it : long_option) {
        joined_options += "--" + it + ",";
        file << "--" << it << std::endl;
    }
    for (auto &it : old_option) {
        joined_options += "-" + it + ",";
        file << "-" << it << std::endl;
    }
    if (description.size() > 3) {
        // Remove last comma
        int last_index = joined_options.size() - 1;
        joined_options.erase(last_index, 1);

        if (joined_options.size() > largest_joined_options_length) {
            largest_joined_options_length = joined_options.size();
        }

        std::vector<std::string> pair = {joined_options, description};
        descriptions.push_back(pair);
    }
};
