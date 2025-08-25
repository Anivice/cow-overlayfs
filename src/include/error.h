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

#endif //HALOKEYBOARD_ERROR_H
