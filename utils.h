#pragma once
#include <string>
#include <filesystem>

inline bool string_ends_with(std::string const& value, std::string const& ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

inline std::string generate_random_string(size_t Size)
{
    static std::string CharSet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string Result;
    while (Result.size() != Size) {
        int pos = rand() % (CharSet.size() - 1);
        Result += CharSet.substr(pos, 1);
    }
    return Result;
}