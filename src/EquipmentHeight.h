#pragma once
#include "HeelsVariables.h"
#include "Actors.h"
#include "AppearanceSettings.h"
#include "dao/Gda.h"
#include "dao/Log.h"
#include <Windows.h>
#include <filesystem>
#include <mutex>
namespace heels {
template<class T> inline bool Copy(std::uintptr_t at,T& out){return at && dao::TryCopy(&out,at,sizeof(out));}
inline std::uintptr_t Equipped(std::uintptr_t creature,int slot){
 std::uintptr_t equipment=0,vt=0,slotsVt=0;
 if(!Copy(creature+0xbec,equipment) || !equipment || !Copy(equipment,vt) || vt!=dao::Module::Address(dao::AddressId{68885}) || !Copy(equipment+0x24,slotsVt) || slotsVt!=dao::Module::Address(dao::AddressId{68886}))return 0;
 using Getter=std::uintptr_t(__fastcall*)(void*,int,const void*);
 const auto getter=reinterpret_cast<Getter>(dao::Module::Address(dao::AddressId{19468}));
 const std::array<std::int32_t,6> key{0,0,slot,-1,-1,-1};
 const auto item=getter(reinterpret_cast<void*>(equipment),0,key.data());
 if(!item)return 0;
 std::uintptr_t itemVt=0;std::uint32_t owner=0,id=0;
 if(!Copy(item,itemVt) || itemVt!=dao::Module::Address(dao::AddressId{68551}) || !Copy(item+0x4a8,owner) || !Copy(creature+0x10,id) || owner!=id)return 0;
 return item;
}
inline bool IsRob(const std::wstring& type){return type.size()==3 && (type[0]==L'r'||type[0]==L'R') && (type[1]==L'o'||type[1]==L'O') && (type[2]==L'b'||type[2]==L'B');}
struct Chest {bool found=false,rob=false;std::uint16_t base=0;std::uint8_t native=0;std::int32_t variation=0;};
inline Chest DescribeChest(std::uintptr_t creature,std::uintptr_t item){
 Chest c;if(!item)return c;
 if(!Copy(item+0x464,c.base) || !Copy(item+0x466,c.native))return {};
 c.variation=c.native;
 // Use the live appearance bridge, not an assumed EV_MODEL_ID interpretation.
 using Query=bool(__cdecl*)(const void*,std::uint32_t,std::int32_t*);
 const auto module=GetModuleHandleW(L"extended_variations.dll");
 const auto query=module?reinterpret_cast<Query>(GetProcAddress(module,"IV_GetAppearanceVariationV1")):nullptr;
 std::uintptr_t appearance=0;std::uint16_t currentBase=0;std::uint8_t currentNative=0;
 if(query && Copy(creature+0x7b0,appearance) && appearance && Copy(appearance+0x12a,currentBase) && Copy(appearance+0x138,currentNative) && currentBase==c.base && currentNative==c.native){
  std::int32_t extended=0;if(query(reinterpret_cast<const void*>(appearance),0,&extended) && extended>255)c.variation=extended;
 }
 const auto table=dao::gda::GetInt(6,c.base,dao::core::GdaColumnHash("Variation_Worksheet"));
 if(!table || *table<=0)return c;
 const auto type=dao::gda::GetString(*table,c.variation,dao::core::GdaColumnHash("ModelType"));
 if(!type)return c;c.found=true;c.rob=IsRob(*type);return c;
}
inline bool BootsVisible(std::uintptr_t torso,const Chest& c,bool fullBodyEnabled){
 if(!torso)return true;
 if(c.base==38 || c.base==22)return false;
 if(!c.found)return false; // Unknown appearance: do not raise potentially hidden boots.
 return !(fullBodyEnabled && c.rob && c.base>=9 && c.base<=12 && c.native>0 && c.native!=255);
}
struct HeightEntry {
 Actor actor{};std::uintptr_t appearance=0;std::int32_t appearanceId=-1;
 ULONGLONG due=0,lastUsed=0;float metres=0;bool appearanceReported=false;std::array<std::uint64_t,6> report{};
};
inline float HeightFor(const Actor& actor,ULONGLONG now=GetTickCount64()){
 // Independent, bounded per-actor polling of two equipped slots only.
 static std::mutex lock;static std::unordered_map<std::uint32_t,HeightEntry> cache;
 std::uintptr_t appearance=0;std::uint16_t appearanceId=0xffff;
 const auto creature=actor.creature;
 // GETAPPEARANCETYPE (ID 13161, GOG 0x65d740) explicitly loads a WORD.
 // The next two bytes are unrelated flags and must not become part of the row ID.
 if(!Copy(creature+0x7b0,appearance) || !appearance || !Copy(appearance+0x58,appearanceId) || appearanceId==0xffff)return 0;
 std::lock_guard guard(lock);
 auto found=cache.find(actor.id);
 if(found==cache.end()){
  if(cache.size()>=1024)cache.erase(std::min_element(cache.begin(),cache.end(),[](const auto&a,const auto&b){return a.second.lastUsed<b.second.lastUsed;}));
  found=cache.emplace(actor.id,HeightEntry{}).first;
 }
 auto& e=found->second;
 if(e.actor!=actor || e.appearance!=appearance || e.appearanceId!=appearanceId){e={};e.actor=actor;e.appearance=appearance;e.appearanceId=appearanceId;}
 e.lastUsed=now;if(now<e.due)return e.metres;e.due=now+200;e.metres=0;
 const auto type=dao::gda::GetString(2,appearanceId,dao::core::GdaColumnHash("ModelType"));
 const bool humanoid=type && type->size()==1 && ((*type)[0]==L'p' || (*type)[0]==L'P');
 if(!e.appearanceReported){e.appearanceReported=true;spdlog::info("APPEARANCE actor={:08x} row={} value_found={} humanoid={}",actor.id,appearanceId,type.has_value(),humanoid);}
 if(!humanoid)return 0;
 const auto torso=Equipped(creature,4),boots=Equipped(creature,6);
 const auto chest=DescribeChest(creature,torso);
 const auto a=ReadHeels(torso),b=ReadHeels(boots);
 const bool visible=BootsVisible(torso,chest,fullBodyEnabled);
 const auto cm=SelectHeight(chest.rob,a,visible,b);e.metres=cm/100.f;
 const std::array<std::uint64_t,6> report{torso,boots,static_cast<unsigned>(a.status),static_cast<unsigned>(b.status),std::bit_cast<std::uint32_t>(cm),static_cast<unsigned>(visible)|(static_cast<unsigned>(chest.rob)<<1)};
 if(report!=e.report){
  e.report=report;
  spdlog::info("ITEM_HEELS actor={:08x} torso={:08x} torso_rob={} torso_status={} torso_cm={:.3f} boots={:08x} boots_visible={} boots_status={} boots_cm={:.3f} selected_cm={:.3f}",actor.id,torso,chest.rob,static_cast<int>(a.status),a.cm,boots,visible,static_cast<int>(b.status),b.cm,cm);
  if(a.status==VariableStatus::Invalid || b.status==VariableStatus::Invalid)spdlog::warn("Invalid HEELS actor={:08x}: requires one unambiguous float, finite and between 0 and 20 cm; invalid values ignored",actor.id);
 }
 return e.metres;
}
}
