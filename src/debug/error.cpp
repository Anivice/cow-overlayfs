/* error.cpp
 *
 * Copyright 2025 Anivice Ives
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <atomic>
#include <vector>
#include <execinfo.h>
#include <sstream>
#include <regex>
#include <cxxabi.h>
#include <iostream>
#include <ranges>
#include "color.h"
#include "execute_command.h"
#include "error.h"
#include "rstring.h"

require_back_trace_t require_back_trace;
#define MAX_STACK_FRAMES (64)

std::string demangle(const char* mangled)
{
    int status = 0;
    char* dem = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
    std::string result = (status == 0 && dem) ? dem : mangled;
    std::free(dem);
    return result;
}

typedef std::vector < std::pair<std::string, void*> > backtrace_info;
backtrace_info obtain_stack_frame()
{
    backtrace_info result;
    void* buffer[MAX_STACK_FRAMES] = {};
    const int frames = backtrace(buffer, MAX_STACK_FRAMES);

    char** symbols = backtrace_symbols(buffer, frames);
    if (symbols == nullptr) {
        return backtrace_info {};
    }

    for (int i = 1; i < frames; ++i) {
        result.emplace_back(symbols[i], buffer[i]);
    }

    free(symbols);
    return result;
}

// fast backtrace
std::string backtrace_level_1()
{
    std::stringstream ss;
    const backtrace_info frames = obtain_stack_frame();
    int i = 0;

#if DEBUG
    auto trim_sym = [](std::string name)->std::string
    {
        name = std::regex_replace(name, std::regex(R"(\(.*\))"), "");
        name = std::regex_replace(name, std::regex(R"(\[abi\:.*\])"), "");
        name = std::regex_replace(name, std::regex(R"(std\:\:.*\:\:)"), "");
        return name;
    };

    auto get_pair = [](const std::string & name)->std::pair<std::string, std::string>
    {
        const std::regex pattern(R"((.*)\((.*)(\+0x.*)\)\s\[.*\])");
        if (std::smatch matches; std::regex_search(name, matches, pattern)) {
            if (matches.size() == 4) {
                return std::make_pair(matches[1].str(),
                    matches[2].str().empty() ? matches[3].str() : demangle(matches[2].str().c_str()));
            }
        }

        return std::make_pair("", name);
    };
#endif

    for (const auto &symbol_name: frames | std::views::keys)
    {
#if DEBUG
        const auto [path, name] = get_pair(symbol_name);
        ss  << color::color(0,4,1) << "    Frame " << color::color(5,2,1) << "#" << i++ << " "
            << std::hex << color::color(2,4,5) << path
            << ": " << color::color(1,5,5) << trim_sym(name) << color::no_color() << "\n";
#else
        ss  << color::color(0,4,1) << " Frame " << color::color(5,2,1) << "#" << i++ << " "
            << symbol_name << color::no_color() << "\n";
#endif
    }

    return ss.str();
}

// slow backtrace, with better trace info
std::string backtrace_level_2()
{
#if DEBUG
    std:: stringstream ss;
    const auto frames = obtain_stack_frame();
    int i = 0;
    const std::regex pattern(R"(([^\(]+)\(([^\)]*)\) \[([^\]]+)\])");
    std::smatch matches;

    struct traced_info
    {
        std::string name;
        std::string file;
    };

    auto generate_addr2line_trace_info = [](const std::string & executable_path, const std::string& address)->traced_info
    {
        const char * env_addr2line_loc_override = getenv("ADDR2LINE_LOC_OVERRIDE");
        auto [fd_stdout, fd_stderr, exit_status]
            = exec_command((env_addr2line_loc_override == nullptr ? "/usr/bin/addr2line" : env_addr2line_loc_override),
                "", "--demangle", "-f", "-p", "-a", "-e",
                executable_path, address);

        if (exit_status != 0)
        {
            std::cerr << "Error when executing addr2line: " << fd_stderr << (!fd_stderr.empty() && fd_stderr.back() == '\n' ? "" : "\n") << std::flush;
            return {};
        }

        std::string caller, path;
        if (const size_t pos = fd_stdout.find('/'); pos != std::string::npos) {
            caller = fd_stdout.substr(0, pos - 4);
            path = fd_stdout.substr(pos);
        }

        if (const size_t pos2 = caller.find('('); pos2 != std::string::npos) {
            caller = caller.substr(0, pos2);
        }

        if (!caller.empty() && !path.empty()) {
            return {.name = caller, .file = replace_all(path, "\n", "") };
        }

        return {.name = replace_all(fd_stdout, "\n", ""), .file = ""};
    };

    for (const auto & [symbol, frame] : frames)
    {
        if (std::regex_search(symbol, matches, pattern) && matches.size() > 3)
        {
            const std::string& executable_path = matches[1].str();
            const std::string& traced_address = matches[2].str();
            const std::string& traced_runtime_address = matches[3].str();
            traced_info info;
            if (traced_address.empty()) {
                info = generate_addr2line_trace_info(executable_path, traced_runtime_address);
            } else {

                info = generate_addr2line_trace_info(executable_path, traced_address);
            }

            ss  << color::color(0,4,1) << "    Frame " << color::color(5,2,1) << "#" << i++ << " "
                << std::hex << color::color(2,4,5) << frame
                << ": " << color::color(1,5,5) << info.name << color::no_color() << "\n";
            if (!info.file.empty())
                ss << "          " << color::color(0,1,5) << info.file << color::no_color() << "\n";
        } else {
            ss << "No trace information for " << std::hex << frame << "\n";
        }
    }

    return ss.str();
#else
    return "!!! Backtrace level 2 not available for release build !!!\n";
#endif

}

std::atomic_int g_pre_defined_level = -1;
class g_pre_defined_level_init_t
{
    public:
    g_pre_defined_level_init_t()
    {
        const char * cppcowoverlay_backtrace_level = std::getenv("CPPCOWOVERLAY_BACKTRACE_LEVEL");
        if (cppcowoverlay_backtrace_level != nullptr)
        {
            try {
                g_pre_defined_level = std::stoi(cppcowoverlay_backtrace_level);
            } catch (...) {
                g_pre_defined_level = 1;
            }
        } else {
            g_pre_defined_level = 1;
        }
    }
} g_pre_defined_level_init;

std::string backtrace()
{
    switch (g_pre_defined_level)
    {
        case 1: return backtrace_level_1();
        case 2: return backtrace_level_2();
        default: return backtrace_level_1();
    }
}

cppCowOverlayBaseErrorType::cppCowOverlayBaseErrorType(const require_back_trace_t&)
    : std::runtime_error(backtrace()) { }

cppCowOverlayBaseErrorType::cppCowOverlayBaseErrorType(const require_back_trace_t&, const std::string &msg)
    : std::runtime_error(color::color(5,0,0) + msg + "\n" + color::color(5,5,0)
        + ">>> BACKTRACE INFORMATION:" + color::no_color() + "\n" +
            backtrace()
        +  color::color(5,5,0) + "<<< BACKTRACE INFORMATION END" + color::no_color()) { }
