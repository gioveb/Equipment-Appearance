// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// DAOLib helper API adapted to this plugin's existing MinHook backend.
#include "dao/Module.h"
#include "MinHook.h"
#include <Windows.h>
#include <spdlog/spdlog.h>
namespace dao {template<class Fn>class Detour {
 Fn original_=nullptr;void* target_=nullptr;
 public:
 bool Attach(AddressId id,Fn hook){return AttachTo(reinterpret_cast<Fn>(Module::Address(id)),hook);}
 bool AttachTo(Fn target,Fn hook){
  if(target_ || !target || !hook)return false;const auto init=MH_Initialize();if(init!=MH_OK && init!=MH_ERROR_ALREADY_INITIALIZED)return false;
  auto* at=reinterpret_cast<void*>(target);Fn trampoline=nullptr;
  const auto status=MH_CreateHook(at,reinterpret_cast<void*>(hook),reinterpret_cast<void**>(&trampoline));if(status!=MH_OK){spdlog::error("EA_GUI_HOOK create failed at={:08x} status={}",reinterpret_cast<std::uintptr_t>(at),MH_StatusToString(status));return false;}
  original_=trampoline;target_=at;
  const auto enabled=MH_EnableHook(at);if(enabled!=MH_OK){spdlog::error("EA_GUI_HOOK enable failed status={}",MH_StatusToString(enabled));MH_RemoveHook(at);target_=nullptr;original_=nullptr;return false;}spdlog::info("EA_GUI_HOOK attached at={:08x}",reinterpret_cast<std::uintptr_t>(at));return true;
 }
 void Detach(){if(target_){MH_DisableHook(target_);MH_RemoveHook(target_);target_=nullptr;original_=nullptr;}}
 Fn Original()const{return original_;}bool IsAttached()const{return target_!=nullptr;}
 ~Detour(){Detach();}
};}
