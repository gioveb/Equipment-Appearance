// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <string_view>

namespace robe {
// 22 is intentionally NOT included: Templar's separate hands must remain visible.
constexpr bool Eligible(std::uint16_t base) noexcept {
    return base == 9 || base == 10 || base == 11 || base == 12;
}
constexpr bool IsRob(std::wstring_view value) noexcept {
    if (value.size() != 3) return false;
    return (value[0] == L'r' || value[0] == L'R') &&
           (value[1] == L'o' || value[1] == L'O') &&
           (value[2] == L'b' || value[2] == L'B');
}
constexpr bool Extend(bool original, bool bodyBuilder, std::uint16_t base,
                      std::int32_t variation, bool found, std::wstring_view type) noexcept {
    if (original) return true;
    return bodyBuilder && Eligible(base) && variation > 0 && variation != 255 && found && IsRob(type);
}
// RAII context, per thread: a recursive appearance rebuild restores its caller.
struct BuildScope {
    void*& current;
    void* previous;
    BuildScope(void*& slot, void* appearance) noexcept : current(slot), previous(slot) { slot = appearance; }
    ~BuildScope() { current = previous; }
    BuildScope(const BuildScope&) = delete;
    BuildScope& operator=(const BuildScope&) = delete;
};
}
