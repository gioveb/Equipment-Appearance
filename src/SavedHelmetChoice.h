// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// CCObject variable pool at +0x2e0, as GETLOCALINT/SETLOCALINT use for creatures.
// Reads are bounded and read-only. Writes call the native allocator/setter on the game thread.
inline constexpr wchar_t helmetChoiceName[]=L"EA_HIDE_HELMET";
bool saveHelmetChoices=false;
struct SavedChoiceCache {std::uintptr_t source=0;ULONGLONG due=0;bool hidden=false;};
std::mutex savedChoiceMutex;std::unordered_map<unsigned,SavedChoiceCache> savedChoices;
std::optional<bool> ReadSavedHelmetChoice(std::uintptr_t actor){
 if(!actor)return std::nullopt;std::optional<bool> result;
 // Only the INT bucket belongs to this preference. Other buckets are unrelated.
 for(unsigned type=1;type<=1;++type){
  const auto v=dao::TryRead<iv::NativeVector>(actor+0x2e0+16*type);
  if(!v || v->end<v->begin || v->capacity<v->end || (v->end-v->begin)%20 || (v->end-v->begin)/20>4096)return std::nullopt;
  if(!v->begin){if(v->end || v->capacity)return std::nullopt;continue;}
  for(auto at=v->begin;at<v->end;at+=20){const auto e=dao::TryRead<iv::IntegerVariable>(at);if(!e)return std::nullopt;const auto name=iv::VariableName(*e);if(!name)continue;
   if(*name!=helmetChoiceName)continue;if(result || type!=1 || (e->value!=0 && e->value!=1))return std::nullopt;result=e->value==1;
  }
 }return result.value_or(false);
}
bool PersistentHelmetHidden(unsigned id,bool fresh=false){
 const auto source=Live(id);if(!source)return false;const auto now=GetTickCount64();
 {std::lock_guard guard(savedChoiceMutex);const auto found=savedChoices.find(id);if(!fresh && found!=savedChoices.end() && found->second.source==source && now<found->second.due)return found->second.hidden;}
 const auto value=ReadSavedHelmetChoice(source);
 {std::lock_guard guard(savedChoiceMutex);if(savedChoices.size()>4096)savedChoices.clear();savedChoices[id]={source,now+250,value.value_or(false)};}
 return value.value_or(false);
}
extern "C" bool __cdecl EA_SaveHelmetPreferenceV1(unsigned id,bool hidden){
 if(!saveHelmetChoices)return true;const auto source=Live(id);if(!source || !ReadSavedHelmetChoice(source)){spdlog::warn("HELMET_PREFERENCE_WRITE rejected actor={:08x} reason=invalid_int_bucket",id);return false;}
 // Native intern explicitly supports a null pool, selecting the global names.
 // CVirtualMachine is normally absent outside script execution.
 std::uintptr_t pool=0;
 struct Key {std::uint32_t pool=0,index=0;} key;
 using Intern=void*(__fastcall*)(void*,int,void*,const wchar_t*,unsigned);
 using SetInt=void(__fastcall*)(void*,int,const void*,int,unsigned,unsigned);
 const auto intern=dao::Module::Address(dao::AddressId{5504}),setter=dao::Module::Address(dao::AddressId{16032});if(!intern || !setter)return false;
 reinterpret_cast<Intern>(intern)(&key,0,reinterpret_cast<void*>(pool),helmetChoiceName,1);
 reinterpret_cast<SetInt>(setter)(reinterpret_cast<void*>(source+0x2e0),0,&key,hidden?1:0,1,1);
 const auto after=ReadSavedHelmetChoice(source);const bool ok=after && *after==hidden;
 spdlog::info("HELMET_PREFERENCE_WRITE actor={:08x} hidden={} verified={} native_local_int=EA_HIDE_HELMET",id,hidden,ok);return ok;
}

// During load DAO normally updates only variables declared by the creature template.
// Permit creation for this plugin-owned name only; forward every other call unchanged.
using SavedIntFn=void(__fastcall*)(void*,int,const void*,int,unsigned,unsigned);
extern "C" {SavedIntFn EA_SavedIntOriginal __asm__("EA_SavedIntOriginal")=nullptr;}
extern "C" bool __cdecl EA_AllowSavedHelmetKey(std::uintptr_t,unsigned) noexcept __asm__("EA_AllowSavedHelmetKey");
extern "C" __attribute__((used,noinline)) bool __cdecl EA_AllowSavedHelmetKey(std::uintptr_t key,unsigned create) noexcept {
 try{if(!saveHelmetChoices || create)return false;
  const auto pair=dao::TryRead<std::array<std::uint32_t,2>>(key);if(!pair)return false;
  iv::IntegerVariable entry{(*pair)[0],(*pair)[1],1,0,0};const auto name=iv::VariableName(entry);
  const bool ours=name && *name==helmetChoiceName;
  if(ours)spdlog::info("HELMET_PREFERENCE_LOAD allow_create=true name=EA_HIDE_HELMET");return ours;
 }catch(...){return false;}
}
__attribute__((used,naked,noinline)) void __fastcall SavedHelmetInt(void*,int,const void*,int,unsigned,unsigned) __asm__("EA_SavedHelmetInt");
__attribute__((used,naked,noinline)) void __fastcall SavedHelmetInt(void*,int,const void*,int,unsigned,unsigned){
 __asm__ volatile(
  "pushfl\n\tpushal\n\tmovl %esp,%ebp\n\tsubl $528,%esp\n\tandl $-16,%esp\n\tfxsave (%esp)\n\t"
  "leal -8(%esp),%esp\n\tpushl 48(%ebp)\n\tpushl 40(%ebp)\n\tcall EA_AllowSavedHelmetKey\n\tleal 16(%esp),%esp\n\t"
  "testb %al,%al\n\tjz 1f\n\tmovl $1,48(%ebp)\n\t1:\n\t"
  "fxrstor (%esp)\n\tmovl %ebp,%esp\n\tpopal\n\tpopfl\n\tjmp *EA_SavedIntOriginal\n\t"
 );
}
