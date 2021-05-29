#include "ZshConverter.hpp"

void ZshConverter::convert(std::string &command, std::string &description,
                           std::vector<std::string> &short_option,
                           std::vector<std::string> &long_option,
                           std::vector<std::string> &old_option) {
    std::string options = "";
    std::string options_begin = "";
    std::string options_end = "";
    int num_delimiters = 0;
    int num_options = 0;

    num_options += short_option.size() + long_option.size() + old_option.size();
    if (num_options == 0) {
        description = "";
        short_option.clear();
        long_option.clear();
        old_option.clear();

        return;
    }
    if (num_options > 1) {
        options_begin = "{";
        options_end = "}";
        num_delimiters = num_options - 1;
    }
    for (auto &it : short_option) {
        options += "-" + it;
        if (num_delimiters > 0) {
            options += ",";
            num_delimiters--;
        }
    }
    for (auto &it : long_option) {
        options += "--" + it;
        if (num_delimiters > 0) {
            options += ",";
            num_delimiters--;
        }
    }
    for (auto &it : old_option) {
        options += "-" + it;
        if (num_delimiters > 0) {
            options += ",";
            num_delimiters--;
        }
    }

    std::string completion_begin = "";
    std::string completion_end = "'";
    std::string description_begin = "";
    std::string description_end = "";
    if (!description.empty()) {
        if (description.compare(0, 1, "'") == 0) {
            description.erase(0, 1);
        }
        int last_index = description.size() - 1;
        if (description.compare(last_index, 1, "'") == 0) {
            description.erase(last_index, 1);
        }
        if (num_options > 1) {
            description_begin = "'[";
        } else {
            completion_begin = "'";
            description_begin = "[";
        }
        description_end = "]";
    }

    file << 
        completion_begin << 
        options_begin <<
        options <<
        options_end <<
        description_begin << 
        description << 
        description_end << 
        completion_end << 
        std::endl;
};
