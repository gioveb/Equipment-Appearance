// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// The party clone is built once, before native animation/VFX bindings are copied.
struct PartyHelmetContext {std::uintptr_t source=0;unsigned id=0xffffffff;};
thread_local PartyHelmetContext partyHelmetContext;
struct PartyHelmetScope {PartyHelmetContext previous;explicit PartyHelmetScope(PartyHelmetContext current):previous(partyHelmetContext){partyHelmetContext=current;}~PartyHelmetScope(){partyHelmetContext=previous;}};
using PartyCreateFn=std::uintptr_t(__fastcall*)(void*,int,void*);
PartyCreateFn partyCreateOriginal=nullptr;
std::uintptr_t __fastcall PartyCreate(void* self,int edx,void* source){
 const auto at=reinterpret_cast<std::uintptr_t>(source);unsigned id=0xffffffff;
 if(!partyPickerVisibility || !Read(at+0x10,id) || Live(id)!=at)return partyCreateOriginal(self,edx,source);
 PartyHelmetScope scope({at,id});return partyCreateOriginal(self,edx,source);
}
void ObservePartyHelmetBinding(void*){} // No late model rebuilds: they invalidate VFX bone bindings.
extern "C" void __cdecl EA_UpdatePartyHelmetsV1(){}
