#pragma once
#include "ItemVariables.h"
#include <bit>
#include <cmath>
namespace heels {
enum class VariableStatus {Missing,Valid,Invalid};
struct Value {VariableStatus status=VariableStatus::Missing;float cm=0;};
inline bool ValidCm(float cm){return std::isfinite(cm) && cm>=0 && cm<=20;}
inline Value ReadHeels(std::uintptr_t item){
 if(!item)return {};
 const auto vt=dao::TryRead<std::uintptr_t>(item);
 if(!vt || *vt!=dao::Module::Address(dao::AddressId{68551}))return {VariableStatus::Invalid,0};
 Value result;unsigned matches=0;
 // Generic ScriptBackend::Find uses 16-byte vector headers / 20-byte entries.
 // Types 1..5 cover int, float, string, object and location. All must be unambiguous.
 for(unsigned type=1;type<=5;++type){
  const auto v=dao::TryRead<iv::NativeVector>(item+0x2e0+16*type);
  if(!v || v->end<v->begin || v->capacity<v->end || (v->end-v->begin)%20 || (v->end-v->begin)/20>4096)return {VariableStatus::Invalid,0};
  if(v->begin==0){if(v->end || v->capacity)return {VariableStatus::Invalid,0};continue;}
  for(std::uintptr_t at=v->begin;at<v->end;at+=20){
   const auto e=dao::TryRead<iv::IntegerVariable>(at);if(!e)return {VariableStatus::Invalid,0};
   const auto name=iv::VariableName(*e);if(!name)return {VariableStatus::Invalid,0};
   if(*name!=L"HEELS")continue;
   ++matches;
   const float cm=std::bit_cast<float>(e->value);
   if(type!=2 || matches!=1 || !ValidCm(cm))return {VariableStatus::Invalid,0};
   result={VariableStatus::Valid,cm};
  }
 }
 return result;
}
inline float SelectHeight(bool torsoIsRob,Value torso,bool bootsVisible,Value boots){
 if(torsoIsRob && torso.status==VariableStatus::Valid && torso.cm>0)return torso.cm;
 if(bootsVisible && boots.status==VariableStatus::Valid)return boots.cm;
 return 0;
}
}
