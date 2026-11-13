// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "Policy.h"
#include "AppearanceSettings.h"
#include "dao/Module.h"
#include "dao/Gda.h"
#include "dao/Log.h"
#include <Windows.h>
#include <mutex>
#include <unordered_set>
namespace ea_body {
inline void BodyLog(std::wstring_view message)noexcept{try{spdlog::info("FullBody: {}",std::string(message.begin(),message.end()));}catch(...){}}
static_assert(sizeof(void*) == 4, "DAO plugins must be x86");
// Verified in GOG 1.05's executable, 2026-10-18:
// BodyModels: CAppearance thiscall, two pointer parameters, bool in AL, ret 8.
// IsChestClothing: CAppearance thiscall, no stack parameters, bool in AL.
// Both slot getters: thiscall, slot uint8 widened to one stack word, ret 4.
using BodyFn = bool(__fastcall*)(void*, int, void*, void*);
using ClothingFn = bool(__fastcall*)(void*, int);
using BaseFn = std::uint16_t(__fastcall*)(void*, int, std::uint32_t);
using VariationFn = std::uint8_t(__fastcall*)(void*, int, std::uint32_t);
BodyFn originalBody = nullptr;
ClothingFn originalClothing = nullptr;
BaseFn getBase = nullptr;
VariationFn getVariation = nullptr;
thread_local void* building = nullptr;
// Diagnostic only, never a classification cache. GDA failures are retried.
std::mutex reportMutex;
std::unordered_set<std::uint64_t> reported;
std::int32_t EffectiveVariation(void* appearance,std::uint8_t native) {
    // Resolve lazily: plugin load order does not decide whether integration works.
    using Query=bool(__cdecl*)(const void*,std::uint32_t,std::int32_t*);
    const auto module=GetModuleHandleW(L"extended_variations.dll");
    const auto query=module?reinterpret_cast<Query>(GetProcAddress(module,"IV_GetAppearanceVariationV1")):nullptr;
    std::int32_t extended=0;
    return query && query(appearance,0,&extended) && extended>255 ? extended : native;
}

bool __fastcall Clothing(void* appearance, int edx) {
    const bool original = originalClothing(appearance, edx);
    if (original || building != appearance || appearance == nullptr || !heels::fullBodyEnabled) return original;
    try {
        const auto base = getBase(appearance, 0, 0);
        // Includes explicit Templar compatibility: base 22 never reaches a GDA lookup.
        if (!robe::Eligible(base)) return original;
        const auto nativeVariation = getVariation(appearance, 0, 0);
        if (nativeVariation == 0 || nativeVariation == 255) return original;
        const auto variation = EffectiveVariation(appearance,nativeVariation);
        const auto sheet = dao::gda::GetInt(6, base, dao::core::GdaColumnHash("Variation_Worksheet"));
        if (!sheet || *sheet <= 0) return original;
        const auto type = dao::gda::GetString(*sheet, variation, dao::core::GdaColumnHash("ModelType"));
        const bool result = robe::Extend(original, true, base, variation, type.has_value(), type ? *type : L"");
        const auto key=(static_cast<std::uint64_t>(base)<<32)|static_cast<std::uint32_t>(variation);
        bool first=false;
        if(result){std::lock_guard lock(reportMutex);first=reported.insert(key).second;}
        if (first) {
            BodyLog(L"Full body rob: BaseItem=" + std::to_wstring(base) + L", variation=" +
                       std::to_wstring(variation) + L", worksheet=" + std::to_wstring(*sheet) +
                       L". Gloves and boots visual models suppressed; equipment untouched.");
        }
        return result;
    } catch (...) {
        BodyLog(L"GDA lookup failed; preserving original rendering for this rebuild.");
        return original;
    }
}
bool __fastcall Body(void* appearance, int edx, void* models, void* records) {
    robe::BuildScope scope(building, appearance);
    return originalBody(appearance, edx, models, records);
}

}
