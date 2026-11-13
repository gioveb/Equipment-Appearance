// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Private implementation inside Probe.cpp namespace. Per-instance visual weapon scaling.
weapon_scale::Config weaponConfig;
weapon_scale::Table weaponTable;
struct WeaponProbeBinding {std::uintptr_t model=0,wield=0,sourceModel=0;unsigned slot=0,row=0,gender=255;std::wstring hierarchy;bool attached=false,reported=false,nodeReported=false;std::uint64_t serial=0;std::uintptr_t rootNode=0;double factor=1;bool ready=false,applyReported=false,rejectionReported=false;};
std::atomic<unsigned> weaponLoadCalls{0},weaponAttachCalls{0};
std::atomic<std::uint64_t> weaponSerial{0};
std::atomic<std::uint64_t> weaponAppliedFrames{0},weaponRejectedFrames{0};
std::mutex weaponProbeMutex;
std::unordered_map<std::uintptr_t,WeaponProbeBinding> weaponBindings;
using WeaponPlanKey=std::tuple<std::wstring,unsigned,unsigned>;
std::map<WeaponPlanKey,double> weaponPlanCache;
std::atomic<DWORD> weaponGameThread{0};
thread_local bool weaponResolving=false;
auto weaponResourceRead=&dao::resources::Read;
using WeaponGdaStringFn=std::optional<std::wstring>(*)(std::int32_t,std::int32_t,std::uint32_t);
WeaponGdaStringFn weaponGdaString=&dao::gda::GetString;
WeaponPlanKey WeaponKey(const WeaponProbeBinding& b){auto name=b.hierarchy;for(auto& c:name)c=std::towlower(c);return {name,b.row,b.gender};}
void WeaponResolveBinding(const WeaponProbeBinding& b);

std::optional<std::wstring> WeaponProbeString(std::uintptr_t at){
 const auto s=dao::TryRead<dao::core::WStringRep>(at);if(!s || s->size>260 || s->capacity<s->size)return {};
 std::wstring value(s->size,L'\0');const auto chars=s->capacity>7?reinterpret_cast<std::uintptr_t>(s->data.pointer):at+4;
 if(!dao::TryCopy(value.data(),chars,value.size()*sizeof(wchar_t)))return {};return value;
}
std::uintptr_t WeaponProbeModel(std::uintptr_t wield,unsigned slot){
 if(slot!=3 && slot!=4)return 0;std::uintptr_t holder=0,model=0;
 return Read(wield+0x224+slot*4,holder) && holder && Read(holder+4,model)?model:0;
}
using WeaponLoadProbeFn=std::uintptr_t(__cdecl*)(void*,const void*,const void*,unsigned,unsigned,unsigned);
using WeaponAttachProbeFn=std::uintptr_t(__cdecl*)(void*,void*,unsigned,unsigned);
extern "C" {WeaponLoadProbeFn EA_WeaponLoadOriginal __asm__("EA_WeaponLoadOriginal")=nullptr;WeaponAttachProbeFn EA_WeaponAttachOriginal __asm__("EA_WeaponAttachOriginal")=nullptr;}
extern "C" void __cdecl EA_WeaponLoaded(std::uintptr_t,std::uintptr_t,unsigned,unsigned) noexcept __asm__("EA_WeaponLoaded");
extern "C" __attribute__((used,noinline)) void __cdecl EA_WeaponLoaded(std::uintptr_t wield,std::uintptr_t name,unsigned slot,unsigned result) noexcept {
 try{slot &= 0xffu;const auto call=++weaponLoadCalls;const auto model=WeaponProbeModel(wield,slot);const auto text=WeaponProbeString(name);
  if(call<=64)spdlog::info("WEAPON_PROBE_LOAD call={} wield={:08x} slot={} result={:08x} model={:08x} name_read={} name_length={} accepted={}",call,wield,slot,result,model,bool(text),text?text->size():0,bool((result&255) && model && text && !text->empty()));
  if(!(result&255) || !model || !text || text->empty())return;
  std::uintptr_t sourceRef=0,sourceModel=0;if(Read(wield+0x18,sourceRef) && sourceRef)Read(sourceRef+4,sourceModel);
  std::lock_guard guard(weaponProbeMutex);if(weaponBindings.size()>=4096)weaponBindings.clear();weaponBindings[model]={model,wield,sourceModel,slot,0,255,*text,false,false,false,++weaponSerial};
 }catch(...){}
}
extern "C" void __cdecl EA_WeaponAttaching(std::uintptr_t,std::uintptr_t,unsigned) noexcept __asm__("EA_WeaponAttaching");
extern "C" __attribute__((used,noinline)) void __cdecl EA_WeaponAttaching(std::uintptr_t wield,std::uintptr_t appearance,unsigned slot) noexcept {
 try{slot &= 0xffu;const auto model=WeaponProbeModel(wield,slot);std::uint16_t row=0;std::uint8_t gender=255;
  if(!model || !Read(appearance+0x58,row) || !Read(appearance+0x50,gender))return;
  std::lock_guard guard(weaponProbeMutex);const auto it=weaponBindings.find(model);
  if(it==weaponBindings.end() || it->second.wield!=wield || it->second.slot!=slot)return;
  auto& b=it->second;const bool changed=b.row!=row || b.gender!=gender;b.row=row;b.gender=gender;b.attached=true;b.serial=++weaponSerial;
  if(changed){b.ready=false;b.factor=1;b.reported=false;}b.applyReported=false;b.rejectionReported=false;
  const auto plan=weaponPlanCache.find(WeaponKey(b));if(plan!=weaponPlanCache.end()){b.factor=plan->second;b.ready=true;b.reported=true;}
 }catch(...){}
}
extern "C" void __cdecl EA_WeaponAttached(std::uintptr_t,std::uintptr_t,unsigned,unsigned) noexcept __asm__("EA_WeaponAttached");
extern "C" __attribute__((used,noinline)) void __cdecl EA_WeaponAttached(std::uintptr_t wield,std::uintptr_t appearance,unsigned slot,unsigned result) noexcept {
 try{slot &= 0xffu;const auto call=++weaponAttachCalls;const auto model=WeaponProbeModel(wield,slot);std::uint16_t row=0;std::uint8_t gender=255;
  if(call<=64)spdlog::info("WEAPON_PROBE_ATTACH call={} wield={:08x} slot={} result={:08x} model={:08x}",call,wield,slot,result,model);
  if(!(result&255)){std::lock_guard guard(weaponProbeMutex);const auto it=weaponBindings.find(model);if(it!=weaponBindings.end() && it->second.wield==wield && it->second.slot==slot){it->second.attached=false;it->second.ready=false;it->second.serial=++weaponSerial;}return;}
  if(!model || !Read(appearance+0x58,row) || !Read(appearance+0x50,gender))return;
  std::lock_guard guard(weaponProbeMutex);const auto it=weaponBindings.find(model);if(it==weaponBindings.end() || it->second.wield!=wield || it->second.slot!=slot)return;
  auto& b=it->second;const bool changed=b.row!=row || b.gender!=gender;b.row=row;b.gender=gender;b.attached=true;b.reported=b.ready;b.serial=++weaponSerial;
  if(changed){b.ready=false;b.factor=1;b.reported=false;}b.applyReported=false;b.rejectionReported=false;
 }catch(...){}
}
__attribute__((used,naked,noinline)) std::uintptr_t __cdecl WeaponLoadProbe(void*,const void*,const void*,unsigned,unsigned,unsigned){
 __asm__ volatile(
  "leal -12(%esp),%esp\n\tpushl 36(%esp)\n\tpushl 36(%esp)\n\tpushl 36(%esp)\n\tpushl 36(%esp)\n\tpushl 36(%esp)\n\tpushl 36(%esp)\n\tcall *EA_WeaponLoadOriginal\n\tleal 36(%esp),%esp\n\t"
  "pushfl\n\tpushal\n\tmovl %esp,%ebp\n\tsubl $528,%esp\n\tandl $-16,%esp\n\tfxsave (%esp)\n\t"
  "pushl 28(%ebp)\n\tpushl 52(%ebp)\n\tpushl 44(%ebp)\n\tpushl 40(%ebp)\n\tcall EA_WeaponLoaded\n\tleal 16(%esp),%esp\n\t"
  "fxrstor (%esp)\n\tmovl %ebp,%esp\n\tpopal\n\tpopfl\n\tret\n\t");
}
__attribute__((used,naked,noinline)) std::uintptr_t __cdecl WeaponAttachProbe(void*,void*,unsigned,unsigned){
 __asm__ volatile(
  "pushfl\n\tpushal\n\tmovl %esp,%ebp\n\tsubl $528,%esp\n\tandl $-16,%esp\n\tfxsave (%esp)\n\t"
  "pushl 48(%ebp)\n\tpushl 44(%ebp)\n\tpushl 40(%ebp)\n\tcall EA_WeaponAttaching\n\tleal 12(%esp),%esp\n\t"
  "fxrstor (%esp)\n\tmovl %ebp,%esp\n\tpopal\n\tpopfl\n\t"
  "leal -12(%esp),%esp\n\tpushl 28(%esp)\n\tpushl 28(%esp)\n\tpushl 28(%esp)\n\tpushl 28(%esp)\n\tcall *EA_WeaponAttachOriginal\n\tleal 28(%esp),%esp\n\t"
  "pushfl\n\tpushal\n\tmovl %esp,%ebp\n\tsubl $528,%esp\n\tandl $-16,%esp\n\tfxsave (%esp)\n\t"
  "pushl 28(%ebp)\n\tpushl 48(%ebp)\n\tpushl 44(%ebp)\n\tpushl 40(%ebp)\n\tcall EA_WeaponAttached\n\tleal 16(%esp),%esp\n\t"
  "fxrstor (%esp)\n\tmovl %ebp,%esp\n\tpopal\n\tpopfl\n\tret\n\t");
}
void WeaponProbeNode(void* node,const void* parent,const void* model,const void* inverse){
 if(!weaponConfig.ProbeEnabled())return;const auto at=reinterpret_cast<std::uintptr_t>(node);std::uintptr_t owner=0;
 if(!Read(at+0x154,owner) || !owner || owner>UINTPTR_MAX-8)return;
 WeaponProbeBinding b;{std::lock_guard guard(weaponProbeMutex);const auto it=weaponBindings.find(owner+8);if(it==weaponBindings.end() || it->second.nodeReported)return;b=it->second;it->second.nodeReported=true;}
 std::uintptr_t definition=0;std::string nodeName; if(Read(at+0x174,definition) && definition){const auto name=WeaponProbeString(definition+0xc0);if(name)for(const auto c:*name)nodeName.push_back(c<128?static_cast<char>(c):'?');}
 heels::Matrix p{},m{},inv{};if(!Read(reinterpret_cast<std::uintptr_t>(parent),p) || !Read(reinterpret_cast<std::uintptr_t>(model),m) || !Read(reinterpret_cast<std::uintptr_t>(inverse),inv))return;
 spdlog::info("WEAPON_SCALE_NODE model={:08x} wield={:08x} slot={} node={} parent_xyz={:.4f},{:.4f},{:.4f} model_xyz={:.4f},{:.4f},{:.4f} parent_basis={:.4f},{:.4f},{:.4f} model_basis={:.4f},{:.4f},{:.4f} inverse_finite={} native_frame_observation=true",b.model,b.wield,b.slot,nodeName,p[3],p[7],p[11],m[3],m[7],m[11],p[0],p[5],p[10],m[0],m[5],m[10],std::all_of(inv.begin(),inv.end(),[](float x){return std::isfinite(x);}));
}
bool WeaponScaleNodeFrame(void* node,const void* parent,const void* model,const void* inverse,heels::Frame& out){
 if(!weaponConfig.tableEnabled && !weaponConfig.creatureEnabled)return false;
 const auto at=reinterpret_cast<std::uintptr_t>(node);std::uintptr_t owner=0;
 if(!Read(at+0x154,owner) || !owner || owner>UINTPTR_MAX-8)return false;
 WeaponProbeBinding b;{std::lock_guard guard(weaponProbeMutex);const auto it=weaponBindings.find(owner+8);
  if(it==weaponBindings.end() || !it->second.attached)return false;b=it->second;}
 if(!b.ready){
  if(weaponResolving || weaponGameThread.load()!=GetCurrentThreadId())return false;
  WeaponResolveBinding(b);
  std::lock_guard guard(weaponProbeMutex);const auto it=weaponBindings.find(b.model);
  if(it==weaponBindings.end() || it->second.serial!=b.serial || !it->second.ready)return false;b=it->second;
 }
 if(b.factor==1)return false;
 // Do not apply an old plan to a freed/replaced weapon or character model.
 if(WeaponProbeModel(b.wield,b.slot)!=b.model)return false;
 std::uintptr_t sourceRef=0,sourceModel=0;
 if(!Read(b.wield+0x18,sourceRef) || !sourceRef || !Read(sourceRef+4,sourceModel) || sourceModel!=b.sourceModel)return false;
 if(b.rootNode){if(b.rootNode!=at)return false;}
 else{std::uintptr_t definition=0;if(!Read(at+0x174,definition) || !definition)return false;
  const auto name=WeaponProbeString(definition+0xc0);if(!name || *name!=L"GOB")return false;}
 heels::Matrix p{},m{},inv{};
 if(!Read(reinterpret_cast<std::uintptr_t>(parent),p) || !Read(reinterpret_cast<std::uintptr_t>(model),m) || !Read(reinterpret_cast<std::uintptr_t>(inverse),inv))return false;
 if(!weapon_scale::ScaleFrame(p,m,inv,b.factor,out)){
  ++weaponRejectedFrames;
  bool report=false;{std::lock_guard guard(weaponProbeMutex);const auto it=weaponBindings.find(b.model);if(it!=weaponBindings.end() && it->second.serial==b.serial){report=!it->second.rejectionReported;it->second.rejectionReported=true;}}
  if(report){float delta=0,error=0;const auto product=heels::Multiply(m,inv);for(unsigned i=0;i<16;++i){delta=std::max(delta,std::abs(p[i]-m[i]));error=std::max(error,std::abs(product[i]-(i%5==0?1.f:0.f)));}
   spdlog::warn("WEAPON_SCALE_SKIPPED model={:08x} slot={} factor={:.4f} reason=root_frame_validation parent_model_delta={:.6f} inverse_error={:.6f} native_inputs_preserved=true",b.model,b.slot,b.factor,delta,error);}
  return false;
 }
 bool report=false;{std::lock_guard guard(weaponProbeMutex);const auto it=weaponBindings.find(b.model);
  if(it==weaponBindings.end() || it->second.serial!=b.serial)return false;
  it->second.rootNode=at;report=!it->second.applyReported;it->second.applyReported=true;}
 ++weaponAppliedFrames;
 if(report){std::uintptr_t controller=0;Read(at+0x15c,controller);
  spdlog::info("WEAPON_SCALE_APPLIED model={:08x} slot={} factor={:.4f} pivot=local_origin world_translation_preserved=true shared_mesh_modified=false native_model_reference_preserved=true node_controller={:08x}",b.model,b.slot,b.factor,controller);}
 return true;
}
std::optional<std::string> WeaponMeshName(const std::vector<std::byte>& b){
 const auto u32=[&](std::size_t at)->unsigned{if(at>b.size() || b.size()-at<4)throw std::runtime_error("MMH bounds");unsigned v=0;std::memcpy(&v,b.data()+at,4);return v;};
 if(b.size()<44 || std::memcmp(b.data(),"GFF V4.0PC  MMH ",16)!=0)return {};
 const auto data=u32(24),fields=u32(36),count=u32(32);if(count>1024)return {};
 for(unsigned i=0;i<count;++i){const auto at=static_cast<std::size_t>(fields)+12*i;if(u32(at)!=6005)continue;
  if(u32(at+4)!=14)return {};const auto pointer=u32(static_cast<std::size_t>(data)+u32(at+8));if(pointer==0xffffffff)return {};const auto p=static_cast<std::size_t>(data)+pointer;const auto length=u32(p);if(length>260)return {};
  std::string name;for(unsigned c=0;c<length;++c){unsigned short ch=0;const auto off=p+4+2*c;if(off>b.size() || b.size()-off<2)return {};std::memcpy(&ch,b.data()+off,2);if(!ch)break;if(ch>127)return {};name.push_back(static_cast<char>(ch));}
  return weapon_scale::ResourceKey(name);
 }return {};
}
void WeaponProbeFrame(){
 weaponGameThread.store(GetCurrentThreadId());
 static ULONGLONG last=0;const auto now=GetTickCount64();if(!last || now-last>=15000){last=now;std::lock_guard guard(weaponProbeMutex);spdlog::info("WEAPON_PROBE_STATUS revision=11 load_calls={} attach_calls={} bindings={} applied_frames={} rejected_frames={} visual_scaling=true",weaponLoadCalls.load(),weaponAttachCalls.load(),weaponBindings.size(),weaponAppliedFrames.load(),weaponRejectedFrames.load());}
 std::vector<WeaponProbeBinding> pending;{std::lock_guard guard(weaponProbeMutex);for(auto& [key,b]:weaponBindings){(void)key;if(b.attached && !b.reported && pending.size()<4){pending.push_back(b);b.reported=true;}}}
 for(const auto& b:pending)WeaponResolveBinding(b);
}
void WeaponResolveBinding(const WeaponProbeBinding& b){
 if(weaponResolving)return;
 struct Scope{Scope(){weaponResolving=true;}~Scope(){weaponResolving=false;}} scope;
 try{
  // The native loader expands a model base through %ls_%d.mmh, %ls.mmh,
  // and %ls_0.mmh. Read LOD0 as a reference; do not claim it is the active LOD.
  std::vector<std::wstring> candidates;if(b.hierarchy.ends_with(L".mmh"))candidates.push_back(b.hierarchy);
  else{candidates.push_back(b.hierarchy+L"_0.mmh");candidates.push_back(b.hierarchy+L".mmh");}
  std::optional<std::string> mesh;std::string resolved,attempts,hash;
  for(const auto& hierarchy:candidates){const std::string shown(hierarchy.begin(),hierarchy.end());const auto bytes=weaponResourceRead(hierarchy);
   if(!attempts.empty())attempts+=",";attempts+=shown+":"+(bytes?std::to_string(bytes->size()):"missing");
   if(bytes){mesh=WeaponMeshName(*bytes);if(mesh){resolved=shown;break;}}}
  spdlog::info("WEAPON_PROBE_RESOURCE base={} reference_mmh={} attempts={} mesh={} resolution=lod0_or_unsuffixed_reference active_lod_unverified=true",std::string(b.hierarchy.begin(),b.hierarchy.end()),resolved,attempts,mesh.value_or("unresolved"));
  if(mesh && weaponConfig.tableEnabled){const std::wstring wide(mesh->begin(),mesh->end());const auto original=weaponResourceRead(wide);if(original)hash=daoaddr::ToHex(daoaddr::Sha256(*original));}
  std::string code;const auto type=weaponGdaString(2,b.row,dao::core::GdaColumnHash("ModelType"));const auto race=weaponGdaString(2,b.row,dao::core::GdaColumnHash("ModelRace"));const auto sex=weaponGdaString(92,b.gender,dao::core::GdaColumnHash("Gender"));
  if(type && (*type==L"P" || *type==L"p") && race && race->size()==1 && sex && sex->size()==1){for(const auto c:{race->front(),sex->front()})code.push_back(static_cast<char>(std::towupper(c)));}
  const auto decision=weapon_scale::Resolve(weaponConfig,weaponTable,mesh.value_or(""),hash,code);
  {std::lock_guard guard(weaponProbeMutex);const auto it=weaponBindings.find(b.model);
   if(it!=weaponBindings.end() && it->second.serial==b.serial){it->second.factor=decision.final;it->second.ready=true;it->second.reported=true;}}
  if(mesh && (!weaponConfig.tableEnabled || !weaponTable.meshes.contains(*mesh) || !hash.empty())){std::lock_guard guard(weaponProbeMutex);if(weaponPlanCache.size()>=4096)weaponPlanCache.clear();weaponPlanCache[WeaponKey(b)]=decision.final;}
  spdlog::info("WEAPON_SCALE_PLAN slot={} model={:08x} source_model={:08x} appearance={} gender={} creature={} mesh={} table_factor={:.4f} creature_factor={:.4f} final_factor={:.4f} table_reason={} unknown_fallback={} darkspawn={} creature_reason={} plan_ready=true",b.slot,b.model,b.sourceModel,b.row,b.gender,code,mesh.value_or("unresolved"),decision.table,decision.creature,decision.final,decision.tableReason,(weaponConfig.tableEnabled && mesh.has_value() && (decision.tableReason=="not_listed" || decision.tableReason=="reference_mismatch")),decision.darkspawn,decision.creatureReason);
 }catch(const std::exception& e){spdlog::warn("WEAPON_SCALE_PROBE skipped: {}",e.what());}
}

void LoadWeaponProbeConfig(const std::filesystem::path& ini){
 weaponConfig.tableEnabled=GetPrivateProfileIntW(L"WeaponScaling",L"TableEnabled",0,ini.c_str())!=0;
 weaponConfig.creatureEnabled=GetPrivateProfileIntW(L"WeaponScaling",L"CreatureEnabled",0,ini.c_str())!=0;
 weaponConfig.excludeDarkspawnFromCreatureScale=GetPrivateProfileIntW(L"WeaponScaling",L"ExcludeDarkspawnFromCreatureScale",1,ini.c_str())!=0;
 weaponConfig.diagnostics=GetPrivateProfileIntW(L"WeaponScaling",L"Diagnostics",0,ini.c_str())!=0;
 wchar_t unknownValue[64]{};GetPrivateProfileStringW(L"WeaponScaling",L"UnknownWeaponScale",L"1.0",unknownValue,64,ini.c_str());wchar_t* unknownEnd=nullptr;
 const double unknown=wcstod(unknownValue,&unknownEnd);const bool validUnknown=unknownEnd!=unknownValue && !*unknownEnd && weapon_scale::ValidScale(unknown);
 weaponConfig.unknownWeaponScale=validUnknown?unknown:1.;
 if(!validUnknown)spdlog::warn("WEAPON_SCALING invalid UnknownWeaponScale; using 1.0 (allowed range 0.05 to 10)");
 for(const auto key:{L"HF",L"HM",L"EF",L"EM",L"DF",L"DM",L"QF",L"QM"}){
  wchar_t value[64]{};GetPrivateProfileStringW(L"WeaponCreatureScale",key,L"1.0",value,64,ini.c_str());wchar_t* end=nullptr;const double n=wcstod(value,&end);const bool valid=end!=value && !*end && weapon_scale::ValidScale(n);std::wstring wide(key);weaponConfig.creature[std::string(wide.begin(),wide.end())]=valid?n:1.0;
 }
 if(weaponConfig.tableEnabled)try{
  wchar_t file[512]{};GetPrivateProfileStringW(L"WeaponScaling",L"TableFile",L"dalls_weapon_scales.json",file,512,ini.c_str());const auto path=ini.parent_path()/file;
  std::ifstream input(path,std::ios::binary|std::ios::ate);if(!input || input.tellg()<0 || input.tellg()>4*1024*1024)throw std::runtime_error("table missing or too large");const auto size=static_cast<std::size_t>(input.tellg());std::string text(size,'\0');input.seekg(0);if(!input.read(text.data(),size))throw std::runtime_error("table read failed");weaponTable=weapon_scale::Parse(text);
 }catch(const std::exception& e){spdlog::warn("WEAPON_SCALE_TABLE unavailable: {}; creature factor remains independent",e.what());}
 spdlog::info("WEAPON_SCALING table={} creature={} entries={} diagnostics={} exclude_darkspawn_creature={} unknown_scale={:.4f} VISUAL TEST: per-instance root transforms; shared mesh bytes unchanged",weaponConfig.tableEnabled,weaponConfig.creatureEnabled,weaponTable.meshes.size(),weaponConfig.diagnostics,weaponConfig.excludeDarkspawnFromCreatureScale,weaponConfig.unknownWeaponScale);
}
