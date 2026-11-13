// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Private namespace. Scoped visual suppression; preserve native machine state.
using BuildProbeFn=std::uintptr_t(__cdecl*)(void*,void*,unsigned,unsigned);
extern "C" { BuildProbeFn EA_BuildOriginal __asm__("EA_BuildOriginal")=nullptr; }
std::mutex buildProbeMutex;
std::unordered_map<std::uint64_t,ULONGLONG> buildProbeReports;
extern "C" void __cdecl EA_ObserveBuild(void*,void*,unsigned,unsigned,std::uintptr_t) noexcept __asm__("EA_ObserveBuild");
extern "C" __attribute__((used,noinline)) void __cdecl EA_ObserveBuild(void* appearance,void* models,unsigned deferred,unsigned id,std::uintptr_t caller) noexcept {
 try{BeginHiddenHelmetBuild(appearance,id);}catch(...){}
 try {
  const auto at=reinterpret_cast<std::uintptr_t>(appearance);std::uint16_t base=0xffff;std::uint8_t variation=255;
  if(!at || !Read(at+0x134,base) || !Read(at+0x13d,variation))return;
  const auto source=Live(id);std::uintptr_t native=0;const auto head=source?heels::Equipped(source,5):0;
  if(source)Read(source+0x7b0,native);
  if(!head && base==0xffff)return;
  const auto key=(static_cast<std::uint64_t>(id)<<32) ^ caller ^ static_cast<std::uint64_t>(at==native);
  const auto now=GetTickCount64();{
   std::lock_guard guard(buildProbeMutex);if(buildProbeReports.size()>256)buildProbeReports.clear();auto& due=buildProbeReports[key];if(now<due)return;due=now+2000;
  }
  const auto stage=Stage();
  spdlog::info("HELMET_BUILD_PROBE actor={:08x} appearance={:08x} source={:08x} source_appearance={:08x} is_source={} head={:08x} base={} variation={} models={:08x} deferred={} caller_rva={:08x} dialogue={} scoped_visual_policy=true",id,at,source,native,at==native,head,base,variation,reinterpret_cast<std::uintptr_t>(models),deferred,caller-dao::Module::Base(),stage.mode && !stage.ending && now<stage.expires);
 }catch(...){}
}
__attribute__((used,naked,noinline)) std::uintptr_t __cdecl HelmetBuildProbe(void*,void*,unsigned,unsigned) __asm__("EA_HelmetBuildProbe");
__attribute__((used,naked,noinline)) std::uintptr_t __cdecl HelmetBuildProbe(void*,void*,unsigned,unsigned) {
 __asm__ volatile(
  "pushfl\n\tpushal\n\tmovl %esp,%ebp\n\tsubl $528,%esp\n\tandl $-16,%esp\n\tfxsave (%esp)\n\t"
  "leal -12(%esp),%esp\n\tpushl 36(%ebp)\n\tpushl 52(%ebp)\n\tpushl 48(%ebp)\n\tpushl 44(%ebp)\n\tpushl 40(%ebp)\n\t"
  "call EA_ObserveBuild\n\tleal 32(%esp),%esp\n\tfxrstor (%esp)\n\tmovl %ebp,%esp\n\tpopal\n\tpopfl\n\t"
  "leal -12(%esp),%esp\n\tpushl 28(%esp)\n\tpushl 28(%esp)\n\tpushl 28(%esp)\n\tpushl 28(%esp)\n\tcall *EA_BuildOriginal\n\tleal 28(%esp),%esp\n\t"
  "pushfl\n\tpushal\n\tmovl %esp,%ebp\n\tsubl $528,%esp\n\tandl $-16,%esp\n\tfxsave (%esp)\n\tcall EA_EndHiddenBuild\n\tfxrstor (%esp)\n\tmovl %ebp,%esp\n\tpopal\n\tpopfl\n\tret\n\t"
 );
}
