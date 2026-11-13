// SPDX-License-Identifier: GPL-3.0-or-later
// Read-only GOG-layout ScriptBackend integer variables. No VM calls or writes.
// Evidence: GETLOCALINT 0x655fa0 selects CCObject+0x2e0; 0x702e90 selects
// type 1; 0x702d50 walks 20-byte entries in 16-byte MSVC vector headers;
// 0x702760 resolves a key's pool/index to a name node's WStringRep at +0xc.
#pragma once
#include "dao/Module.h"
#include "dao/SafeRead.h"
#include "dao/core/Msvc8.h"
#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>

namespace iv {
struct NativeVector {std::uint32_t proxy,begin,end,capacity;};
struct IntegerVariable {std::uint32_t pool,index,type;std::int32_t value;std::uint32_t flags;};
static_assert(sizeof(NativeVector)==16 && sizeof(IntegerVariable)==20);
inline std::optional<std::wstring> VariableName(const IntegerVariable& entry) {
    const auto pool=entry.pool?static_cast<std::uintptr_t>(entry.pool):
        dao::Module::Address(dao::AddressId{100262});
    if(!pool)return std::nullopt;
    const auto names=dao::TryRead<NativeVector>(pool);
    if(!names || !names->begin || names->end<names->begin || names->capacity<names->end ||
       (names->end-names->begin)%4 || (names->end-names->begin)/4>65536 ||
       entry.index>=(names->end-names->begin)/4) return std::nullopt;
    const auto node=dao::TryRead<std::uint32_t>(names->begin+entry.index*4);
    if(!node || !*node)return std::nullopt;
    const auto address=static_cast<std::uintptr_t>(*node)+12;
    const auto rep=dao::TryRead<dao::core::WStringRep>(address);
    if(!rep || !rep->size || rep->size>128 || rep->capacity<rep->size || rep->capacity>0x100000)
        return std::nullopt;
    const auto chars=rep->capacity>7?reinterpret_cast<std::uintptr_t>(rep->data.pointer):address+4;
    std::wstring name(rep->size,L'\0');
    if(!dao::TryCopy(name.data(),chars,name.size()*sizeof(wchar_t)))return std::nullopt;
    return name;
}
inline std::optional<std::int32_t> ItemModelVariable(void* item) {
    if(!item)return std::nullopt;
    const auto address=reinterpret_cast<std::uintptr_t>(item);
    const auto vtable=dao::TryRead<std::uintptr_t>(address);
    if(!vtable || *vtable!=dao::Module::Address(dao::AddressId{68551}))return std::nullopt;
    const auto variables=dao::TryRead<NativeVector>(address+0x2e0+0x10);
    if(!variables || !variables->begin || variables->end<variables->begin ||
       variables->capacity<variables->end || (variables->end-variables->begin)%20 ||
       (variables->end-variables->begin)/20>4096)return std::nullopt;
    std::optional<std::int32_t> found;
    for(std::uintptr_t p=variables->begin;p<variables->end;p+=20) {
        const auto entry=dao::TryRead<IntegerVariable>(p);
        if(!entry)return std::nullopt;
        const auto name=VariableName(*entry);
        if(!name || *name!=L"EV_MODEL_ID")continue;
        // Duplicate definitions are ambiguous, even when their current values agree.
        if(found)return std::nullopt;
        found=entry->value;
    }
    return found;
}
}
