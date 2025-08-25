#include <filesystem>
#include <fstream>
#include <regex>
#include "rstring.h"
#include "configuration.h"

std::string clean_line(const std::string& line)
{
    return line.substr(0, line.find_first_of('#'));
}

std::string get_section(const std::string& line)
{
    const std::regex pattern(R"(\s*\[([^\]]+)\]\s*)");
    if (std::smatch matches; std::regex_match(line, matches, pattern) && matches.size() > 1)
    {
        return matches[1];
    }

    return "";
}

std::pair <std::string, std::string> get_pair(const std::string& line)
{
    std::pair <std::string, std::string> pair;
    const std::regex pattern(R"(\s*([^=]+)\s*=\s*(.*)\s*)");
    if (std::smatch matches; std::regex_match(line, matches, pattern) && matches.size() > 2)
    {
        pair.first = matches[1];
        pair.second = matches[2];
    }

    pair.first = pair.first.substr(0, std::min(pair.first.find_last_not_of(' ') + 1, pair.first.size())); // remove tailing spaces
    pair.second = pair.second.substr(0, std::min(pair.second.find_last_not_of(' ') + 1, pair.second.size()));
    return pair;
}

std::string process_value(std::string value)
{
    return regex_replace_all(value, R"(\%[\w]+\%)",
        [](const std::string & word)->std::string
        {
            std::string result = word;
            const char * env = std::getenv(replace_all(result, "%", "").c_str());
            if (env == nullptr) {
                return "";
            }
            return env;
        }
    );
}

configuration::configuration(const std::string& path)
{
    std::ifstream file(path);
    if (!file) {
        throw InvalidConfiguration("Cannot open config file " + path);
    }

    std::string line;
    std::string section;
    int line_num = 0;
    while (std::getline(file, line))
    {
        line_num++;
        std::string section_tmp = get_section(clean_line(line));
        const auto [key, value] = get_pair(clean_line(line));
        if (!section_tmp.empty())
        {
            section = section_tmp;
        }
        else
        {
            if (!key.empty())
            {
                if (section.empty()) {
                    throw InvalidConfiguration("Line: " + std::to_string(line_num) + ": section head is empty");
                }
                config_[section][key].push_back(process_value(value));
            }
        }
    }
}
