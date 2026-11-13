// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Included in the private namespace. The machine bridge preserves the entire
// native register/flags/FX state around C++ observers, including ECX and EDX.
bool showHelmetDialog=true;
using SceneMaskFn=std::uintptr_t(__fastcall*)(void*,int,void*);
extern "C" {SceneMaskFn EA_MaskOriginal __asm__("EA_MaskOriginal")=nullptr;}
struct HelmetLease {std::uintptr_t scene=0,appearance=0,source=0,item=0;std::uint32_t id=0xffffffff;};
thread_local std::array<HelmetLease,16> helmetLeases{};
thread_local unsigned helmetLeaseDepth=0;
extern "C" void __cdecl EA_MaskBegin(void*,void*) noexcept __asm__("EA_MaskBegin");
extern "C" void __cdecl EA_MaskEnd() noexcept __asm__("EA_MaskEnd");
extern "C" __attribute__((used,noinline)) void __cdecl EA_MaskBegin(void* object,void* appearance) noexcept {
 const auto index=helmetLeaseDepth++;if(index>=helmetLeases.size())return;auto& lease=helmetLeases[index];lease={};
 try{
  if(!object || !appearance)return;const auto scene=reinterpret_cast<std::uintptr_t>(object);std::uint32_t id=0xffffffff;
  // The native scene mask is the scope: dialogue lifetime/focus can change
  // while the engine is already preparing the next cutscene actor.
  if(!Read(scene+0x1c0,id) || id==0xffffffff || (!showHelmetDialog && !HelmetHidden(id)))return;
  const auto source=Live(id);if(!source)return;const auto item=heels::Equipped(source,5);if(!item)return;
  lease={scene,reinterpret_cast<std::uintptr_t>(appearance),source,item,id};
 }catch(...){}
}
std::mutex helmetDialogMutex;std::unordered_map<std::uint32_t,ULONGLONG> helmetDialogReports;
extern "C" __attribute__((used,noinline)) void __cdecl EA_MaskEnd() noexcept {
 if(!helmetLeaseDepth)return;const auto index=--helmetLeaseDepth;if(index>=helmetLeases.size())return;const auto lease=helmetLeases[index];helmetLeases[index]={};
 try{
  if(!lease.item)return;std::uint32_t id=0xffffffff;
  if(!Read(lease.scene+0x1c0,id) || id!=lease.id || Live(id)!=lease.source || heels::Equipped(lease.source,5)!=lease.item)return;
  if(HelmetHidden(lease.id)){
   if(ClearVisualHelmet(lease.appearance))spdlog::info("HELMET_DIALOG_HIDE actor={:08x} equipment_unchanged=true",lease.id);
   return;
  }
  if(!showHelmetDialog)return;
  std::uint16_t base=0;std::uint8_t variation=0;
  if(!Read(lease.appearance+0x134,base) || !Read(lease.appearance+0x13d,variation) || (base!=0xffff && variation!=255))return;
  const auto address=dao::Module::Address(dao::AddressId{17790});if(!address)return;
  using Transfer=void(__fastcall*)(void*,int,std::uint32_t,void*);
  // Use the existing entry point, including Extended Variations' own detour.
  // This restores all native helmet metadata and EV state, not just two bytes.
  reinterpret_cast<Transfer>(address)(reinterpret_cast<void*>(lease.appearance),0,5,reinterpret_cast<void*>(lease.item));
  std::uint16_t afterBase=0xffff;std::uint8_t afterVariation=255;Read(lease.appearance+0x134,afterBase);Read(lease.appearance+0x13d,afterVariation);
  const auto now=GetTickCount64();bool report=false;{std::lock_guard guard(helmetDialogMutex);if(helmetDialogReports.size()>=128 && !helmetDialogReports.contains(lease.id))helmetDialogReports.clear();auto& due=helmetDialogReports[lease.id];if(now>=due){due=now+2000;report=true;}}
  if(report)spdlog::info("HELMET_DIALOG_RESTORE actor={:08x} appearance={:08x} item={:08x} before_base={} before_variation={} after_base={} after_variation={} equipment_unchanged=true",lease.id,lease.appearance,lease.item,base,variation,afterBase,afterVariation);
 }catch(...){}
}
__attribute__((used,naked,noinline)) std::uintptr_t __fastcall HelmetSceneMask(void*,int,void*) __asm__("EA_HelmetSceneMask");
__attribute__((used,naked,noinline)) std::uintptr_t __fastcall HelmetSceneMask(void*,int,void*) {
 __asm__ volatile(
  "pushfl\n\tpushal\n\tmovl %esp,%ebp\n\tsubl $528,%esp\n\tandl $-16,%esp\n\tfxsave (%esp)\n\t"
  "leal -8(%esp),%esp\n\tpushl 40(%ebp)\n\tpushl 24(%ebp)\n\tcall EA_MaskBegin\n\tleal 16(%esp),%esp\n\t"
  "fxrstor (%esp)\n\tmovl %ebp,%esp\n\tpopal\n\tpopfl\n\t"
  // Preserve the native entry alignment (16 bytes below the original entry).
  "leal -8(%esp),%esp\n\tpushl 12(%esp)\n\tcall *EA_MaskOriginal\n\tleal 8(%esp),%esp\n\t"
  "pushfl\n\tpushal\n\tmovl %esp,%ebp\n\tsubl $528,%esp\n\tandl $-16,%esp\n\tfxsave (%esp)\n\t"
  "call EA_MaskEnd\n\tfxrstor (%esp)\n\tmovl %ebp,%esp\n\tpopal\n\tpopfl\n\tret $4\n\t"
 );
}
