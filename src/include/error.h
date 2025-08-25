#ifndef HALOKEYBOARD_ERROR_H
#define HALOKEYBOARD_ERROR_H

#include <stdexcept>

class require_back_trace_t {};
extern require_back_trace_t require_back_trace;

class cppCowOverlayBaseErrorType : public std::runtime_error
{
    public:
        cppCowOverlayBaseErrorType() : std::runtime_error("") {}
        explicit cppCowOverlayBaseErrorType(const std::string &msg) : std::runtime_error(msg) {}
        explicit cppCowOverlayBaseErrorType(const require_back_trace_t&);
        explicit cppCowOverlayBaseErrorType(const require_back_trace_t&, const std::string &msg);
};

#define _error_h_lstr(x) #x
#define _error_h_str(x) _error_h_lstr(x)

#define cow_assert(condition, type) if (!(condition)) { throw type("Assert " #condition " failed at " __FILE__ ":" _error_h_str(__LINE__)); }
#define cow_assert_wm(condition, type, message) if (!(condition)) { throw type(std::string(message) + (DEBUG ? ": " #condition " at " __FILE__ ":" _error_h_str(__LINE__) : "")); }

// define a simple exception from the base class
#define def_except_no_trace(name)                                                                                       \
    class name final : public cppCowOverlayBaseErrorType {                                                              \
        public: explicit name(const std::string & msg) : cppCowOverlayBaseErrorType(#name ": " + msg) {}                \
    }

#endif //HALOKEYBOARD_ERROR_H
