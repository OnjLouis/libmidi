#pragma once
#include <cstdio>
#include <string>

// Only the two utility operations used by the SMF/container test target.
namespace msc
{
template <typename T> bool InRange(T value, T first, T last)
{
    return value >= first && value <= last;
}
template <typename... Args> std::string FormatText(const char * format, Args... args)
{
    if constexpr (sizeof...(Args) == 0) return format;
    else
    {
    const int size = std::snprintf(nullptr, 0, format, args...);
    std::string text(static_cast<size_t>(size), '\0');
    std::snprintf(text.data(), text.size() + 1, format, args...);
    return text;
    }
}
}
