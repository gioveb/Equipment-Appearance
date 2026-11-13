// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Included in the plugin's private namespace. No native slot getter is hooked.
bool helmetDiagnostics=true;
struct HelmetSnapshot {
 std::uint16_t row=0xffff,base=0xffff;std::uint8_t variation=255,fullFace=0;
 std::uint32_t actor=0xffffffff;std::uintptr_t appearance=0,source=0,clone=0,head=0,cloak=0;
 bool dialogue=false,scripted=false;
 bool operator==(const HelmetSnapshot&)const=default;
};
struct HelmetSample {HelmetSnapshot state{};ULONGLONG at=0;};
std::mutex helmetMutex;std::unordered_map<std::uintptr_t,HelmetSample> helmetSamples;
bool ReadHelmetSnapshot(std::uintptr_t source,std::uintptr_t visual,HelmetSnapshot& out){
 if(!source || !visual || !Read(source+0x10,out.actor) || Live(out.actor)!=source || !Read(visual+0x7b0,out.appearance) || !out.appearance)return false;
 std::uintptr_t sourceVt=0,visualVt=0;
 if(!Read(source,sourceVt) || !sourceVt || !Read(visual,visualVt) || sourceVt!=visualVt)return false;
 if(!Read(out.appearance+0x58,out.row) || !Read(out.appearance+0x134,out.base) || !Read(out.appearance+0x13d,out.variation) || !Read(out.appearance+0x5e,out.fullFace))return false;
 out.source=source;out.clone=visual==source?0:visual;
 // Two equipped-slot reads only; no inventory scanning and no writes.
 out.head=heels::Equipped(source,5);out.cloak=heels::Equipped(source,8);
 const auto stage=Stage();out.dialogue=stage.mode && !stage.ending && GetTickCount64()<stage.expires;out.scripted=out.dialogue && stage.scripted;
 return true;
}
void ReportHelmetSnapshot(const char* context,std::uintptr_t source,std::uintptr_t visual){
 if(!helmetDiagnostics)return;HelmetSnapshot state;if(!ReadHelmetSnapshot(source,visual,state))return;
 const auto key=visual;const auto now=GetTickCount64();
 {std::lock_guard guard(helmetMutex);const auto it=helmetSamples.find(key);if(it!=helmetSamples.end() && it->second.state==state && now>=it->second.at && now-it->second.at<5000)return;
  if(helmetSamples.size()>=512 && !helmetSamples.contains(key))helmetSamples.erase(std::min_element(helmetSamples.begin(),helmetSamples.end(),[](const auto&a,const auto&b){return a.second.at<b.second.at;}));helmetSamples[key]={state,now};}
 spdlog::info("HELMET_SNAPSHOT context={} appearance={:08x} actor={:08x} clone={:08x} dialogue={} scripted={} row={} base={} native_variation={} full_face_field={} equipped_head={:08x} equipped_cloak={:08x} diagnostic_only=true",context,state.appearance,state.actor,state.clone,state.dialogue,state.scripted,state.row,state.base,state.variation,state.fullFace,state.head,state.cloak);
}
void ObserveHelmetInventory(void* object){
 if(!helmetDiagnostics)return;const auto owner=reinterpret_cast<std::uintptr_t>(object);std::uintptr_t vt=0,source=0,clone=0;
 if(!Read(owner,vt) || vt!=dao::Module::Address(dao::AddressId{69083}) || !Read(owner+0x58,source) || !Read(owner+0x68,clone) || !clone || source==clone)return;
 ReportHelmetSnapshot("inventory",source,clone);
}
void ObserveHelmetParty(void* object){
 if(!helmetDiagnostics)return;const auto owner=reinterpret_cast<std::uintptr_t>(object);std::uint32_t id=0xffffffff;std::uintptr_t clone=0;
 if(!Read(owner,id) || !Read(owner+4,clone) || !clone)return;const auto source=Live(id);if(!source || source==clone)return;
 ReportHelmetSnapshot("party_picker",source,clone);
}
