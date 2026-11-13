// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
namespace equipment {
constexpr std::uint8_t kHelmetMask=0x08;
constexpr std::uint8_t ShowHelmet(std::uint8_t mask){return mask|kHelmetMask;}
constexpr std::uint8_t RestoreHelmetBit(std::uint8_t current,std::uint8_t original){return (current & ~kHelmetMask)|(original & kHelmetMask);}
}
