#ifndef CPPCOWOVERLAY_RSTRING_H
#define CPPCOWOVERLAY_RSTRING_H

#include <string>
#include <functional>
std::string replace_all(std::string & original, const std::string & target, const std::string & replacement);
std::string regex_replace_all(std::string & original, const std::string & pattern, const std::function<std::string(const std::string &)>& replacement);

#endif //CPPCOWOVERLAY_RSTRING_H
