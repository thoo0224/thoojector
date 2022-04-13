#pragma once
#include <string>
#include <random>

inline bool string_ends_with(std::string const& value, std::string const& ending)
{
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

inline std::string generate_random_string(size_t length = 0)
{
    static const std::string allowed_chars{ "123456789BCDFGHJKLMNPQRSTVWXZbcdfghjklmnpqrstvwxz" };

    static thread_local std::default_random_engine randomEngine(std::random_device{}());
    static thread_local std::uniform_int_distribution<int> randomDistribution(0, allowed_chars.size() - 1);

    std::string id(length ? length : 32, '\0');

    for (std::string::value_type& c : id) {
        c = allowed_chars[randomDistribution(randomEngine)];
    }

    return id;
}