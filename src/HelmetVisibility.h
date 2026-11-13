// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Internal render policy only: equipment slots/items are never written.
#include "SavedHelmetChoice.h"
bool partyPickerVisibility=true;
std::mutex hiddenHelmetMutex;
std::unordered_set<std::uint32_t> hiddenHelmetActors;
bool HelmetHidden(std::uint32_t id){if(saveHelmetChoices)return PersistentHelmetHidden(id);std::lock_guard guard(hiddenHelmetMutex);return hiddenHelmetActors.contains(id);}
struct VisualHelmetFields {std::uint16_t base=0xffff;std::uint8_t variation=255,fullFace=0;};
bool ReadVisualHelmet(std::uintptr_t at,VisualHelmetFields& f){return Read(at+0x134,f.base) && Read(at+0x13d,f.variation) && Read(at+0x5e,f.fullFace);}
template<class T>bool HelmetWrite(std::uintptr_t at,const T& value){SIZE_T written=0;return WriteProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(at),&value,sizeof(value),&written) && written==sizeof(value);}
bool WriteVisualHelmet(std::uintptr_t at,const VisualHelmetFields& f){
 return HelmetWrite(at+0x134,f.base) && HelmetWrite(at+0x13d,f.variation) && HelmetWrite(at+0x5e,f.fullFace);
}
bool ClearVisualHelmet(std::uintptr_t at){VisualHelmetFields old;if(!ReadVisualHelmet(at,old))return false;const bool ok=WriteVisualHelmet(at,{});if(!ok)WriteVisualHelmet(at,old);return ok;}
void LoadHiddenHelmets(const std::filesystem::path& ini){
 partyPickerVisibility=GetPrivateProfileIntW(L"Helmets",L"PartyPickerVisibility",1,ini.c_str())!=0;
 saveHelmetChoices=GetPrivateProfileIntW(L"Helmets",L"SaveCharacterPreference",1,ini.c_str())!=0;
 wchar_t text[2048]{};GetPrivateProfileStringW(L"HelmetVisibilityTest",L"HiddenActors",L"",text,2048,ini.c_str());
 wchar_t* current=text;while(*current){wchar_t* end=nullptr;const auto id=wcstoul(current,&end,16);if(end==current)break;if(id!=0xffffffff)hiddenHelmetActors.insert(static_cast<std::uint32_t>(id));current=end;while(*current==L',' || *current==L' ' || *current==L';')++current;}
 spdlog::info("HELMET_VISIBILITY_TEST hidden_actor_count={}; initial per-actor policy; equipment-slot movement OFF; runtime quickbar control enabled",hiddenHelmetActors.size());
}
#include "PartyHelmetChoice.h"
struct BuildHelmetLease {std::uintptr_t appearance=0;VisualHelmetFields old{};};
thread_local std::array<BuildHelmetLease,16> buildHelmetLeases{};
thread_local unsigned buildHelmetDepth=0;
void BeginHiddenHelmetBuild(void* appearance,unsigned id){
 const auto index=buildHelmetDepth++;if(index>=buildHelmetLeases.size())return;auto& lease=buildHelmetLeases[index];lease={};
 const auto at=reinterpret_cast<std::uintptr_t>(appearance);unsigned policyId=id;auto source=Live(id);
 const bool partyCreation=partyHelmetContext.source && Live(partyHelmetContext.id)==partyHelmetContext.source;
 if(partyCreation){policyId=partyHelmetContext.id;source=partyHelmetContext.source;}
 // A native build must see the loaded value now, even if this actor ID/address
 // still has a cached choice from another save. Never schedule another build.
 const bool hidden=saveHelmetChoices?PersistentHelmetHidden(policyId,true):HelmetHidden(policyId);
 const auto item=source?heels::Equipped(source,5):0;
 if(partyCreation && item && !hidden){
  const auto transfer=dao::Module::Address(dao::AddressId{17790});
  if(transfer){using Transfer=void(__fastcall*)(void*,int,std::uint32_t,void*);
   reinterpret_cast<Transfer>(transfer)(appearance,0,5,reinterpret_cast<void*>(item));
   spdlog::info("HELMET_PARTY_INIT source_actor={:08x} appearance={:08x} visible=true before_vfx_copy=true",policyId,at);}
 }
 if(!hidden || !source || !item)return;
 VisualHelmetFields fields;if(!ReadVisualHelmet(at,fields) || fields.base==0xffff)return;
 // GUI clones observed in 0.0.6 have their own IDs and no equipped head item.
 // The hidden-ID policy therefore never applies to those copied previews.
 if(!ClearVisualHelmet(at))return;lease={at,fields};
 spdlog::info("HELMET_WORLD_HIDE actor={:08x} appearance={:08x} item={:08x} base={} variation={} equipment_unchanged=true",id,at,item,fields.base,fields.variation);
}
extern "C" void __cdecl EA_EndHiddenBuild() noexcept __asm__("EA_EndHiddenBuild");
extern "C" __attribute__((used,noinline)) void __cdecl EA_EndHiddenBuild() noexcept {
 if(!buildHelmetDepth)return;const auto index=--buildHelmetDepth;if(index>=buildHelmetLeases.size())return;const auto lease=buildHelmetLeases[index];buildHelmetLeases[index]={};
 if(lease.appearance && !WriteVisualHelmet(lease.appearance,lease.old))spdlog::error("HELMET_WORLD_HIDE failed to restore appearance fields");
}

extern "C" __declspec(dllexport) bool __cdecl EA_HelmetHiddenV1(unsigned id){return HelmetHidden(id);}
extern "C" __declspec(dllexport) void __cdecl EA_SetHelmetHiddenV1(unsigned id,bool hidden){
 {std::lock_guard guard(hiddenHelmetMutex);if(hidden)hiddenHelmetActors.insert(id);else hiddenHelmetActors.erase(id);}
 if(saveHelmetChoices){std::lock_guard guard(savedChoiceMutex);savedChoices[id]={Live(id),GetTickCount64()+250,hidden};}
}

extern "C" unsigned __cdecl EA_ControlledV1(){
 std::uintptr_t app=0,holder=0,party=0;unsigned id=0xffffffff;
 if(!Read(dao::Module::Address(dao::AddressId{100835}),app) || !app || !Read(app+0xd0,holder) || !holder || !Read(holder+0x58,party) || !party || !Read(holder+0x5c,id))return 0xffffffff;return Live(id)?id:0xffffffff;
}
extern "C" bool __cdecl EA_HasHeadV1(unsigned id){const auto ptr=Live(id);return ptr && heels::Equipped(ptr,5);}
extern "C" bool __cdecl EA_RebuildHelmetV1(unsigned id){
 const auto ptr=Live(id);std::uintptr_t appearance=0;std::uint8_t ready=0;
 if(!ptr || !Read(ptr+0x7b0,appearance) || !appearance || !Read(ptr+0x7b4,ready) || !ready)return false;
 const auto item=heels::Equipped(ptr,5);if(!item)return false;
 using Rebuild=bool(__fastcall*)(void*,int,void*,unsigned,unsigned,unsigned);
 const auto address=dao::Module::Address(dao::AddressId{14969});if(!address)return false;
 const bool result=reinterpret_cast<Rebuild>(address)(reinterpret_cast<void*>(ptr),0,reinterpret_cast<void*>(appearance),0,0,0);
 const auto after=heels::Equipped(ptr,5);spdlog::info("HELMET_REBUILD actor={:08x} result={} head_unchanged={}",id,result,item==after);return result && item==after;
}

// Native loading already builds the model. An extra early rebuild can run before
// tint initialization finishes. Only an explicit UI click requests a rebuild.
extern "C" void __cdecl EA_RefreshSavedHelmetsV1(){}
