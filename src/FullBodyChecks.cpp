// SPDX-License-Identifier: GPL-3.0-or-later
// Exercises production robe callbacks with a mock engine and a separately loaded bridge provider.
// Tests the calling conventions, detours, SDK GDA/string calls and address file.
#include "FullBody.h"
#include "EquipmentHeight.h"
using namespace ea_body;
#include "dao/core/Msvc8.h"
#include "dao/core/GdaHash.h"
#include "daoaddr/Library.h"
#include <Windows.h>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <cstring>
#include <string>

struct Appearance {
    std::array<std::byte,0x12a> padding{};
    std::array<std::uint16_t,7> base{};
    std::array<std::uint8_t,7> variation{};
};
static_assert(offsetof(Appearance, base) == 0x12a && offsetof(Appearance, variation) == 0x138);
unsigned checks = 0, lookups = 0;
bool found = true;
bool bridgeMode=false;
int lastRow=-1;
std::wstring modelType = L"rob";
void Check(bool b) { ++checks; if (!b) { std::cerr << "FAIL check " << checks << '\n'; std::exit(1); } }

__attribute__((naked,noinline)) bool __fastcall MockClothing(Appearance*, int) {
    __asm__ volatile(".byte 0x33,0xc0,0x66,0x83,0xb9,0x2a,0x01,0x00,0x00,0x26,0x0f,0x94,0xc0,0xc3");
}
__attribute__((noinline)) std::uint16_t __fastcall GetBase(Appearance* p, int, std::uint32_t slot) { return p->base.at(slot); }
__attribute__((noinline)) std::uint8_t __fastcall GetVariation(Appearance* p, int, std::uint32_t slot) { return p->variation.at(slot); }
using ClothingCall = bool(__fastcall*)(Appearance*,int);
ClothingCall volatile clothingCall = reinterpret_cast<ClothingCall>(&Clothing);
__attribute__((noinline)) bool __fastcall NativeBody(Appearance* p, int, void* models, void*) {
    // Exactly the native part-count decision, with a non-inlined call to the predicate.
    *static_cast<int*>(models) = clothingCall(p,0) ? 1 : (p->base[0] == 22 ? 2 : 3);
    return true;
}
__attribute__((noinline)) bool __fastcall GdaInt(void*, int, int table, int row, std::uint32_t hash, int* out) {
    ++lookups;
    if (!found || table != 6 || row < 9 || row > 12 || hash != dao::core::GdaColumnHash("Variation_Worksheet")) return false;
    *out = row == 9 ? 28 : row == 10 ? 27 : row == 11 ? 29 : 26;
    return true;
}
__attribute__((noinline)) bool __fastcall GdaString(void*, int, int table, int row, std::uint32_t hash, dao::core::WStringRep* out) {
    ++lookups;
    if (!found || table < 26 || table > 29 || hash != dao::core::GdaColumnHash("ModelType")) return false;
    lastRow=row;
    const auto text=bridgeMode && row>255 ? (row==5000 || row==65536?std::wstring(L"rob"):std::wstring(L"arm")) : modelType;
    out->size = static_cast<std::uint32_t>(text.size());
    if (modelType.size() <= 7) {
        out->capacity = 7;
        text.copy(out->data.buffer,text.size()); out->data.buffer[text.size()] = 0;
    } else {
        auto* heap = static_cast<wchar_t*>(HeapAlloc(GetProcessHeap(),0,(modelType.size()+1)*sizeof(wchar_t)));
        std::memcpy(heap, text.c_str(),(text.size()+1)*sizeof(wchar_t));
        out->data.pointer = heap; out->capacity = out->size;
    }
    return true;
}
__attribute__((noinline)) void __fastcall Destroy(dao::core::WStringRep* p, int) {
    if (p->capacity > 7) HeapFree(GetProcessHeap(),0,const_cast<wchar_t*>(p->data.pointer));
    p->size = 0; p->capacity = 7; p->data.buffer[0] = 0;
}
__attribute__((noinline)) dao::core::WStringRep* __fastcall Assign(dao::core::WStringRep* p, int, const dao::core::WStringRep*) { return p; }
struct Keeper { void* unused; void* store; } keeper{nullptr, reinterpret_cast<void*>(1)};
void* tables = &keeper;


namespace dao {
bool TryCopy(void*out,std::uintptr_t address,std::size_t size) noexcept {SIZE_T n=0;return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),out,size,&n)&&n==size;}
namespace Module {std::uintptr_t Address(AddressId id){switch(static_cast<std::uint32_t>(id)){
case 99739:return reinterpret_cast<std::uintptr_t>(&tables);
case 32473:return reinterpret_cast<std::uintptr_t>(&GdaInt);
case 32471:return reinterpret_cast<std::uintptr_t>(&GdaString);
case 4498:return reinterpret_cast<std::uintptr_t>(&Destroy);
case 4546:return reinterpret_cast<std::uintptr_t>(&Assign);
default:return 0;}}}
}
int wmain(int argc, wchar_t** argv) {
    Check(argc == 2);
    originalBody=reinterpret_cast<ea_body::BodyFn>(&NativeBody);
    originalClothing=reinterpret_cast<ea_body::ClothingFn>(&MockClothing);
    getBase=reinterpret_cast<ea_body::BaseFn>(&GetBase);
    getVariation=reinterpret_cast<ea_body::VariationFn>(&GetVariation);
    using BodyFn=bool(__fastcall*)(Appearance*,int,void*,void*);
    BodyFn volatile bodyCall=reinterpret_cast<BodyFn>(&Body);
    Appearance appearance; appearance.variation[0]=7;
    for (const auto id : {9,10,11,12,22,38,0,8}) {
        appearance.base[0]=static_cast<std::uint16_t>(id);
        modelType=L"rob"; int parts=0;
        const auto before=lookups;
        Check(bodyCall(&appearance,0,&parts,nullptr));
        Check(parts == ((id>=9&&id<=12)||id==38 ? 1 : id==22 ? 2 : 3));
        Check(clothingCall(&appearance,0)==(id==38)); // Outside builder: untouched.
        if(id==22) Check(lookups==before); // Templar never consults robe classification.
    }
    for(const auto type : {L"arm",L"cth",L"rob_extra",L"ROB",L"rob",L""}) {
        appearance.base[0]=10; modelType=type; int parts=0;
        Check(bodyCall(&appearance,0,&parts,nullptr));
        Check(parts == (modelType==L"rob"||modelType==L"ROB"?1:3));
    }
    // A failed early lookup must not be cached as a negative forever.
    appearance.base[0]=10; modelType=L"rob"; found=false; int parts=0;
    Check(bodyCall(&appearance,0,&parts,nullptr)&&parts==3);
    found=true; Check(bodyCall(&appearance,0,&parts,nullptr)&&parts==1);
    for(const auto variation : {0,255}) {
        appearance.variation[0]=static_cast<std::uint8_t>(variation);
        Check(bodyCall(&appearance,0,&parts,nullptr)&&parts==3);
    }
    // Load the query provider AFTER the robe plugin to exercise load-order independence.
    appearance.base[0]=10;appearance.variation[0]=1;modelType=L"arm";
    Check(bodyCall(&appearance,0,&parts,nullptr)&&parts==3);
    const auto bridgeDll=LoadLibraryW(argv[1]);Check(bridgeDll!=nullptr);
    using SetVariation=void(__cdecl*)(std::int32_t);
    const auto set=reinterpret_cast<SetVariation>(GetProcAddress(bridgeDll,"TestSetVariation"));Check(set!=nullptr);
    bridgeMode=true;
    set(5000);Check(bodyCall(&appearance,0,&parts,nullptr)&&parts==1 && lastRow==5000);
    set(65536);Check(bodyCall(&appearance,0,&parts,nullptr)&&parts==1 && lastRow==65536);
    set(5001);modelType=L"rob";Check(bodyCall(&appearance,0,&parts,nullptr)&&parts==3 && lastRow==5001);
    modelType=L"arm";set(0);Check(bodyCall(&appearance,0,&parts,nullptr)&&parts==3 && lastRow==1);
    set(255);Check(bodyCall(&appearance,0,&parts,nullptr)&&parts==3 && lastRow==1);
    set(5000);appearance.base[0]=22;const auto prior=lookups;
    Check(bodyCall(&appearance,0,&parts,nullptr)&&parts==2 && lookups==prior);
    appearance.base[0]=10;found=false;Check(bodyCall(&appearance,0,&parts,nullptr)&&parts==3);
    found=true;Check(bodyCall(&appearance,0,&parts,nullptr)&&parts==1);
    Check(!clothingCall(&appearance,0));
    heels::Chest chest{true,true,10,7,7};
    Check(!heels::BootsVisible(1,chest,heels::fullBodyEnabled));
    heels::fullBodyEnabled=false;modelType=L"rob";appearance.base[0]=10;
    Check(bodyCall(&appearance,0,&parts,nullptr)&&parts==3);
    Check(heels::BootsVisible(1,chest,heels::fullBodyEnabled));
    heels::fullBodyEnabled=true;
    std::cout << "PASS: " << checks << " production callback checks; cross-DLL bridge, full 5000/65536 IDs, late loading, native fallback, Templar preserved\n";
    // Do not hot-unload a detour DLL; the real loader keeps plugins until process exit too.
    return 0;
}
