// SPDX-License-Identifier: GPL-3.0-or-later
#include "Policy.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main() {
    unsigned checked = 0;
    const auto check = [&](bool result) { ++checked; if (!result) std::abort(); };
    for (unsigned base = 0; base < 256; ++base) {
        const auto b = static_cast<std::uint16_t>(base);
        const bool eligible = base >= 9 && base <= 12;
        check(robe::Eligible(b) == eligible);
        for (const auto type : {L"rob", L"ROB", L"rOb", L"arm", L"cth", L"rob_tmp", L"", L" rob"}) {
            const bool match = robe::IsRob(type);
            check(robe::Extend(false, true, b, 7, true, type) == (eligible && match));
            check(!robe::Extend(false, false, b, 7, true, type));
            check(!robe::Extend(false, true, b, 7, false, type));
            check(robe::Extend(true, true, b, 7, true, type));
            check(!robe::Extend(false, true, b, 0, true, type));
            check(!robe::Extend(false, true, b, 255, true, type));
        }
    }
    // Templar stays chest+gloves; its separate no-boots predicate is never hooked.
    for (unsigned variation = 0; variation < 256; ++variation)
        check(!robe::Extend(false, true, 22, static_cast<std::uint8_t>(variation), true, L"rob"));
    // Vanilla Chantry A remains clothing; Chantry B becomes a full body medium.
    check(robe::Extend(true, true, 38, 14, true, L"rob"));
    check(robe::Extend(false, true, 10, 7, true, L"rob"));
    void* context = nullptr;
    int a = 0, b = 0;
    { robe::BuildScope outer(context, &a); check(context == &a);
      { robe::BuildScope inner(context, &b); check(context == &b); }
      check(context == &a);
      try { robe::BuildScope inner(context, &b); throw std::runtime_error("test"); } catch (...) {}
      check(context == &a);
    }
    check(context == nullptr);
    std::cout << "PASS: " << checked << " policy and context checks\n";
}
