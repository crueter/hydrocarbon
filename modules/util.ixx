// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

module;

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string_view>

export module hydrocarbon:util;

export namespace hydro::util
{

// Case insensitive string comparison.
inline bool iequals(std::string_view a, std::string_view b)
{
    if (a.size() != b.size())
        return false;

    return std::ranges::equal(a, b, [](char c1, char c2) {
        return std::tolower(uint8_t(c1)) == std::tolower(uint8_t(c2));
    });
}

// Case-insensitive string contains.
inline bool icontains(std::string_view haystack, std::string_view needle)
{
    if (needle.empty())
        return true;

    auto res = std::ranges::search(haystack, needle, [](char a, char b) {
        return std::tolower(uint8_t(a)) == std::tolower(uint8_t(b));
    });
    return !res.empty();
}

} // namespace hydro::util