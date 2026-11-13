// SPDX-License-Identifier: GPL-3.0-or-later
#include "dao/Plugin.h"
#include "WeaponScalePolicy.h"
#include "dao/Resources.h"
#include "dao/Frame.h"
#include "dao/core/Msvc8.h"
#include "daoaddr/CodeIndex.h"
#include <cwctype>
#include "dao/Log.h"
#include "dao/Module.h"
#include "dao/SafeRead.h"
#include "Guards.h"
#include "GuiCodeGuards.h"
#include "MinHook.h"
#include "Offset.h"
#include "WeaponScaleFrame.h"
#include "EquipmentHeight.h"
#include "Actors.h"
#include "DialogueCamera.h"
#include "StagedPose.h"
#include "CinematicCamera.h"
#include "FullBody.h"
#include "HelmetDialogPolicy.h"
#include <Windows.h>
#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <cstring>

void InstallHelmetQuickbar(const std::filesystem::path&);
namespace dao {bool TryCopy(void* out,std::uintptr_t at,std::size_t size) noexcept {SIZE_T n=0;return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(at),out,size,&n) && n==size;}}
namespace {
template<class T> bool Read(std::uintptr_t at,T& out){return at && dao::TryCopy(&out,at,sizeof(out));}
using MoveFn=void(__fastcall*)(void*,int,void*,unsigned);
using NodeFn=void(__fastcall*)(void*,int,const void*,const void*,const void*);
MoveFn moveOriginal=nullptr;NodeFn nodeOriginal=nullptr;
std::mutex mutex;heels::Actors actors;
std::atomic<bool> active=true;
bool dialogueCompensation=true;
bool inventoryPreview=false;
thread_local heels::CameraContext dialogueCamera;
std::atomic<ULONGLONG> nextKeyPoll=0,nextRefresh=0;
std::atomic<ULONGLONG> dialogueTransitionUntil=0;
std::mutex stageMutex;heels::DialogueState stageState;
heels::DialogueState Stage(){std::lock_guard guard(stageMutex);return stageState;}
std::uint32_t HeroId(){
 std::uintptr_t app=0,context=0,party=0;std::uint32_t id=0xffffffff;
 if(!Read(dao::Module::Address(dao::AddressId{100835}),app) || !app || !Read(app+0xd0,context) || !context || !Read(context+0x58,party) || !party || !Read(party+0x204,id))return 0xffffffff;return id;
}
thread_local unsigned depth=0;
struct LogState {heels::Actor actor{};ULONGLONG due=0;bool raised=false;};
std::mutex logMutex;std::unordered_map<std::uint32_t,LogState> nextLogs;
std::uintptr_t Live(std::uint32_t id){
 std::uintptr_t app=0;if(id==0xffffffff || !Read(dao::Module::Address(dao::AddressId{100835}),app) || !app)return 0;
 using Lookup=std::uintptr_t(__fastcall*)(void*,int,std::uint32_t);
 return reinterpret_cast<Lookup>(dao::Module::Address(dao::AddressId{10509}))(reinterpret_cast<void*>(app),0,id);
}
bool Model(std::uintptr_t creature,std::uintptr_t& result){std::uintptr_t set=0,holder=0;return Read(creature+0x154,set) && set && Read(set+0x18,holder) && holder && Read(holder+4,result) && result;}
std::optional<heels::Actor> Identify(std::uintptr_t creature){
 heels::Actor a;a.creature=creature;
 if(!Read(creature+0x10,a.id) || Live(a.id)!=creature || !Read(creature+0x254,a.area) || !Model(creature,a.model))return {};
 return a;
}
#include "WeaponScaleProbe.h"
void Track(const heels::Actor& a){
 std::lock_guard guard(mutex);if(actors.Upsert(a,GetTickCount64()))spdlog::info("ACTOR object_id={:08x} creature={:08x} model_interface={:08x} area={:08x}",a.id,a.creature,a.model,a.area);
}
void Observe(void* ptr){const auto a=Identify(reinterpret_cast<std::uintptr_t>(ptr));if(a)Track(*a);}
void Refresh(){
 const auto now=GetTickCount64();auto due=nextRefresh.load();if(now<due || !nextRefresh.compare_exchange_strong(due,now+200))return;
 std::vector<heels::Actor> snapshot;{std::lock_guard guard(mutex);snapshot=actors.Snapshot();}
 for(const auto& old:snapshot){
  const auto ptr=Live(old.id);const auto current=ptr?Identify(ptr):std::nullopt;
  if(!current){std::lock_guard guard(mutex);actors.Erase(old);continue;}
  Track(*current);
 }
}
void __fastcall Move(void* self,int edx,void* old,unsigned flag){try{Observe(self);}catch(...){}moveOriginal(self,edx,old,flag);}
void Keys(){
 const auto now=GetTickCount64();auto due=nextKeyPoll.load();if(now<due || !nextKeyPoll.compare_exchange_strong(due,now+50))return;
 DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);if(pid!=GetCurrentProcessId())return;
 static bool held=false;const bool down=(GetAsyncKeyState(VK_F8)&0x8000)!=0;
 if(down && !held){const bool value=!active.load();active.store(value);spdlog::info("AUTOMATIC HEELS {}; humanoid actors; item float in cm",value?"ON":"OFF");}
 held=down;
}
// On a newly encountered model, immediately retry actors known to be raised.
// Only current native object identities are accepted; no inventory scans occur here.
std::optional<heels::Actor> ResolveModel(std::uintptr_t model,const heels::Matrix& frame){
 struct Miss {std::array<std::uint32_t,3> position{};ULONGLONG due=0;};
 static std::unordered_map<std::uintptr_t,Miss> misses;
 const std::array<std::uint32_t,3> position{std::bit_cast<std::uint32_t>(frame[3]),std::bit_cast<std::uint32_t>(frame[7]),std::bit_cast<std::uint32_t>(frame[11])};
 const auto now=GetTickCount64();
 std::vector<heels::Actor> candidates;
 {std::lock_guard guard(mutex);
  const auto known=actors.Find(model);if(known)return known;
  const auto miss=misses.find(model);if(now>=dialogueTransitionUntil.load() && miss!=misses.end() && miss->second.position==position && now<miss->second.due)return {};
  candidates=actors.RaisedCandidates();
 }
 for(const auto& old:candidates){
  std::array<float,4> logical{};
  if(!Read(old.creature+0x120,logical) || std::abs(logical[0]-frame[3])>0.01f || std::abs(logical[1]-frame[7])>0.01f || std::abs(logical[2]-frame[11])>0.01f)continue;
  const auto ptr=Live(old.id);const auto current=ptr?Identify(ptr):std::nullopt;
  if(!current || current->model!=model)continue;
  Track(*current);
  spdlog::info("MODEL_REBOUND_IMMEDIATE actor={:08x} old_model={:08x} new_model={:08x}",current->id,old.model,current->model);
  {std::lock_guard guard(mutex);misses.erase(model);}return current;
 }
 {std::lock_guard guard(mutex);if(misses.size()>=4096 && !misses.contains(model))misses.clear();misses[model]={position,now+250};}
 return {};
}
// GUI preview clones are bound by the engine's own per-frame paperdoll/party entry.
// They never enter the world actor registry or contribute dialogue head samples.
#include "HelmetSnapshots.h"
#include "HelmetVisibility.h"
#include "HelmetDialog.h"
#include "HelmetBuildProbe.h"

enum class PreviewKind {Paperdoll,PartyPicker};
struct PreviewBinding {PreviewKind kind{};std::uintptr_t owner=0,clone=0,model=0;heels::Actor source{};ULONGLONG at=0;};
std::mutex previewMutex;std::unordered_map<std::uintptr_t,PreviewBinding> previews;
std::atomic<ULONGLONG> nextPreviewReport=0;
std::optional<heels::Actor> PreviewSource(std::uintptr_t source){
 heels::Actor a;a.creature=source;
 if(!source || !Read(source+0x10,a.id) || Live(a.id)!=source || !Read(source+0x254,a.area))return {};
 Model(source,a.model);return a;
}
std::optional<PreviewBinding> ReadPreview(PreviewKind kind,std::uintptr_t owner){
 if(kind==PreviewKind::Paperdoll && !inventoryPreview)return {};
 std::uintptr_t source=0,clone=0,vt=0;
 if(kind==PreviewKind::Paperdoll){
  if(!Read(owner,vt) || vt!=dao::Module::Address(dao::AddressId{69083}) || !Read(owner+0x58,source) || !Read(owner+0x68,clone))return {};
 }else{
  std::uint32_t id=0xffffffff;if(!Read(owner,id) || !Read(owner+4,clone))return {};source=Live(id);
 }
 if(!source || !clone || source==clone)return {};
 const auto actor=PreviewSource(source);if(!actor)return {};
 std::uintptr_t sv=0,cv=0,model=0;
 if(!Read(source,sv) || !sv || !Read(clone,cv) || cv!=sv || !Model(clone,model) || model==actor->model)return {};
 return PreviewBinding{kind,owner,clone,model,*actor,GetTickCount64()};
}
void ObservePreview(PreviewKind kind,void* self){
 const auto binding=ReadPreview(kind,reinterpret_cast<std::uintptr_t>(self));if(!binding)return;
 std::lock_guard guard(previewMutex);
 if(previews.size()>=64 && !previews.contains(binding->model))previews.erase(std::min_element(previews.begin(),previews.end(),[](const auto&a,const auto&b){return a.second.at<b.second.at;}));
 previews[binding->model]=*binding;
}
// Creation may update the new model before publishing it at paperdoll+0x68.
// Accept only the current, generation-matched native creation command on this thread.
thread_local std::uintptr_t previewCreation=0;
struct PreviewCreationScope {std::uintptr_t previous;explicit PreviewCreationScope(std::uintptr_t value):previous(previewCreation){previewCreation=value;}~PreviewCreationScope(){previewCreation=previous;}};
std::optional<PreviewBinding> PendingPreview(std::uintptr_t model){
 const auto command=previewCreation;if(!inventoryPreview || !command)return {};
 std::uintptr_t vt=0,owner=0,source=0,clone=0,selected=0,ownerVt=0;std::uint32_t generation=0,currentGeneration=0;
 if(!Read(command,vt) || vt!=dao::Module::Address(dao::AddressId{69015}) || !Read(command+0x208,owner) || !owner || !Read(owner,ownerVt) || ownerVt!=dao::Module::Address(dao::AddressId{69083}) || !Read(command+0x204,source) || !Read(owner+0x58,selected) || selected!=source || !Read(command+8,generation) || !Read(owner+0x2f0,currentGeneration) || generation!=currentGeneration || !Read(command+0x200,clone) || !clone || clone==source)return {};
 const auto actor=PreviewSource(source);if(!actor)return {};
 std::uintptr_t sourceVt=0,cloneVt=0,currentModel=0;
 if(!Read(source,sourceVt) || !sourceVt || !Read(clone,cloneVt) || cloneVt!=sourceVt || !Model(clone,currentModel) || currentModel!=model || model==actor->model)return {};
 return PreviewBinding{PreviewKind::Paperdoll,owner,clone,model,*actor,GetTickCount64()};
}
using PreviewCreateFn=std::uintptr_t(__fastcall*)(void*,int);
PreviewCreateFn previewCreateOriginal=nullptr;
std::uintptr_t __fastcall PreviewCreate(void* self,int edx){
 PreviewCreationScope scope(reinterpret_cast<std::uintptr_t>(self));
 return previewCreateOriginal(self,edx);
}
std::optional<PreviewBinding> FindPreview(void* node){
 std::uintptr_t owner=0,dependency=0;
 if(!Read(reinterpret_cast<std::uintptr_t>(node)+0x154,owner) || owner>UINTPTR_MAX-8 || !Read(reinterpret_cast<std::uintptr_t>(node)+0x15c,dependency) || dependency)return {};
 const auto model=owner+8;const auto now=GetTickCount64();
 if(const auto pending=PendingPreview(model)){
  static thread_local std::uintptr_t lastCommand=0,lastModel=0;
  if(lastCommand!=previewCreation || lastModel!=model){lastCommand=previewCreation;lastModel=model;spdlog::info("PREVIEW_CREATION_BOUND source_actor={:08x} clone={:08x} model={:08x} before_publication=true",pending->source.id,pending->clone,model);}
  return pending;
 }
 std::vector<PreviewBinding> candidates;
 {std::lock_guard guard(previewMutex);
  const auto found=previews.find(model);if(found!=previews.end())candidates.push_back(found->second);
  // A GUI may replace its clone/model between the native tick and the first draw.
  // Retry fresh owning paperdolls immediately; do not wait for the next GUI tick.
  for(const auto& [key,binding]:previews){
   if(binding.kind!=PreviewKind::Paperdoll || now<binding.at || now-binding.at>1000)continue;
   if(std::none_of(candidates.begin(),candidates.end(),[&](const auto& old){return old.kind==binding.kind && old.owner==binding.owner;}))candidates.push_back(binding);
  }
 }
 for(const auto& binding:candidates){
  if(now<binding.at || now-binding.at>1000)continue;
  const auto current=ReadPreview(binding.kind,binding.owner);
  if(!current || current->model!=model)continue;
  // Direct entries require the same identity. Fresh paperdoll ownership allows
  // a replacement clone or a newly selected actor, proven by current native fields.
  if(binding.kind!=PreviewKind::Paperdoll && (current->clone!=binding.clone || current->source!=binding.source))continue;
  if(current->clone!=binding.clone || current->model!=binding.model || current->source!=binding.source){
   {std::lock_guard guard(previewMutex);
    for(auto it=previews.begin();it!=previews.end();)if(it->second.kind==binding.kind && it->second.owner==binding.owner)it=previews.erase(it);else ++it;
    if(previews.size()>=64 && !previews.contains(model))previews.erase(std::min_element(previews.begin(),previews.end(),[](const auto&a,const auto&b){return a.second.at<b.second.at;}));previews[model]=*current;
   }
   spdlog::info("PREVIEW_REBOUND_IMMEDIATE source_actor={:08x} old_clone={:08x} new_clone={:08x} old_model={:08x} new_model={:08x}",current->source.id,binding.clone,current->clone,binding.model,model);
  }
  return current;
 }
 return {};
}
using PreviewTickFn=std::uintptr_t(__fastcall*)(void*,int,float);
PreviewTickFn paperTickOriginal=nullptr,partyTickOriginal=nullptr;
std::uintptr_t __fastcall PaperTick(void* self,int edx,float dt){
 try{ObservePreview(PreviewKind::Paperdoll,self);}catch(...){}
 const auto result=paperTickOriginal(self,edx,dt);try{ObserveHelmetInventory(self);}catch(...){}try{ObservePreview(PreviewKind::Paperdoll,self);}catch(...){}return result;
}
std::uintptr_t __fastcall PartyTick(void* self,int edx,float dt){
 try{ObservePreview(PreviewKind::PartyPicker,self);}catch(...){}
 const auto result=partyTickOriginal(self,edx,dt);try{ObservePartyHelmetBinding(self);ObserveHelmetParty(self);}catch(...){}try{ObservePreview(PreviewKind::PartyPicker,self);}catch(...){}return result;
}
std::atomic<ULONGLONG> nextSkipReport=0;
struct Context {heels::Actor actor{};bool staged=false;float height=0;std::uintptr_t creature=0;std::array<float,4> logical{};heels::Matrix model{},parent{},inverse{};};
Context Prepare(void* self,const void* parent,const void* model,const void* inverse){
 Context c;std::uintptr_t owner=0,dependency=0;
 if(!Read(reinterpret_cast<std::uintptr_t>(self)+0x154,owner) || owner>UINTPTR_MAX-8)return c;
 if(!Read(reinterpret_cast<std::uintptr_t>(model),c.model))return {};
 {const auto found=ResolveModel(owner+8,c.model);if(!found)return {};c.actor=*found;c.creature=found->creature;}
 if(!Read(reinterpret_cast<std::uintptr_t>(self)+0x15c,dependency))return {};
 if(dependency){return {};}
 if(!c.creature)return {};
 std::uintptr_t current=0;std::uint32_t currentArea=0xffffffff;
 if(c.actor.area==0xffffffff || Live(c.actor.id)!=c.creature || !Read(c.creature+0x254,currentArea) || currentArea!=c.actor.area || !Model(c.creature,current) || current!=owner+8 || !Read(c.creature+0x120,c.logical) || !Read(reinterpret_cast<std::uintptr_t>(parent),c.parent) || !Read(reinterpret_cast<std::uintptr_t>(model),c.model) || !Read(reinterpret_cast<std::uintptr_t>(inverse),c.inverse))return {};
 return c;
}
void Check(void* self,const Context& c,bool applied){
 std::array<float,4> after{};heels::Matrix world{},space{},modelWorld{};
 if(!Read(c.creature+0x120,after) || !Read(reinterpret_cast<std::uintptr_t>(self)+0x60,world) || !Read(reinterpret_cast<std::uintptr_t>(self)+0xa0,space) || !Read(reinterpret_cast<std::uintptr_t>(self)+0x20,modelWorld))return;
 const bool unchanged=std::memcmp(c.logical.data(),after.data(),sizeof(after))==0;
 const auto now=GetTickCount64();bool report=false;{
  std::lock_guard guard(logMutex);if(nextLogs.size()>=1024 && !nextLogs.contains(c.actor.id))nextLogs.clear();
  auto& entry=nextLogs[c.actor.id];if(entry.actor!=c.actor){entry={};entry.actor=c.actor;}
  entry.raised=entry.raised || applied;
  if(entry.raised && now>=entry.due){entry.due=now+2000;report=true;}
 }
 if(!unchanged && applied){std::lock_guard guard(mutex);actors.Block(c.actor);spdlog::error("STOP actor={:08x}: logical position changed across node update; offset blocked for this actor/model",c.actor.id);return;}
 {std::lock_guard guard(mutex);actors.Rendered(c.actor,applied && unchanged?c.height:0.f,now,{c.model[3],c.model[7],c.model[11],1.f});}
 if(report)spdlog::info("OFFSET_CHECK actor={:08x} staged={} applied={} logical_unchanged={} logical_z={:.5f} node_world_z={:.5f} delta_z={:.5f} model_space_z={:.5f} model_world_z={:.5f} reconstructed_world_z={:.5f} requested_metres={:.5f} native_reference_z={:.5f} visual_delta_z={:.5f}",c.actor.id,c.staged,applied,unchanged,after[2],world[11],world[11]-after[2],space[11],modelWorld[11],heels::Multiply(modelWorld,space)[11],c.height,c.model[11],world[11]-c.model[11]);
}
// Observe an already-computed head transform during the normal hierarchy walk.
// No engine head getter or forced graphics/model refresh is called from camera code.
struct HeadSample {heels::Actor actor{};std::array<float,4> native{};ULONGLONG at=0;};
std::unordered_map<std::uint32_t,HeadSample> headSamples;
struct HeadCapture {heels::Actor actor{};std::optional<std::array<float,4>> world;};
thread_local HeadCapture* headCapture=nullptr;
void CaptureComputedHead(void* self){
 if(!headCapture || headCapture->world)return;
 const auto node=reinterpret_cast<std::uintptr_t>(self);std::uintptr_t owner=0,vt=0,getName=0,definition=0;
 if(!Read(node+0x154,owner) || owner+8!=headCapture->actor.model || !Read(node,vt) || !Read(vt+10*4,getName))return;
 if(getName!=dao::Module::Address(dao::AddressId{8310}) && getName!=dao::Module::Address(dao::AddressId{9643}))return;
 if(!Read(node+0x174,definition) || !definition)return;
 dao::core::WStringRep name{};
 if(!Read(definition+0xc0,name) || name.size!=4 || name.capacity<4 || name.capacity>0x100000)return;
 const auto chars=name.capacity>7?reinterpret_cast<std::uintptr_t>(name.data.pointer):definition+0xc4;
 std::array<wchar_t,4> text{};if(!Read(chars,text))return;
 const std::array<wchar_t,4> expected{L'h',L'e',L'a',L'd'};for(unsigned i=0;i<4;++i)if((text[i]|0x20)!=expected[i])return;
 heels::Matrix world{};if(!Read(node+0x60,world))return;
 for(float value:world)if(!std::isfinite(value))return;
 if(std::abs(world[15]-1)>0.001f)return;
 headCapture->world=std::array<float,4>{world[3],world[7],world[11],1};
}
struct HeadCaptureScope {HeadCapture* previous;explicit HeadCaptureScope(HeadCapture* value):previous(headCapture){headCapture=value;}~HeadCaptureScope(){headCapture=previous;}};
void __fastcall Node(void* self,int edx,const void* parent,const void* model,const void* inverse){
 try{WeaponProbeNode(self,parent,model,inverse);}catch(...){}
 const bool root=depth==0;Context c;heels::Frame shifted{};const void* useParent=parent;const void* useModel=model;const void* useInverse=inverse;
 if(depth==0)try{
  Keys();Refresh();const auto preview=FindPreview(self);
  if(preview){
   heels::Matrix p{},m{},inv{};float height=active.load()?heels::HeightFor(preview->source):0.f;bool applied=false;
   if(Read(reinterpret_cast<std::uintptr_t>(parent),p) && Read(reinterpret_cast<std::uintptr_t>(model),m) && Read(reinterpret_cast<std::uintptr_t>(inverse),inv)){
    const std::array<float,4> reference{m[3],m[7],m[11],1};
    if(height>0 && heels::ShiftFrame(p,m,inv,reference,height,shifted)){useParent=shifted.parent.data();useModel=shifted.model.data();useInverse=shifted.inverse.data();applied=true;}
   }
   const auto now=GetTickCount64();auto due=nextPreviewReport.load();if(now>=due && nextPreviewReport.compare_exchange_strong(due,now+2000))spdlog::info("PREVIEW_HEELS kind={} source_actor={:08x} clone={:08x} model={:08x} requested_cm={:.3f} applied={}",preview->kind==PreviewKind::Paperdoll?"inventory":"party_picker",preview->source.id,preview->clone,preview->model,height*100,applied);
  }else c=Prepare(self,parent,model,inverse);
  const bool on=active.load();
  if(c.creature){
   c.height=on?heels::HeightFor(c.actor):0.f;
   if(!heels::ShiftPose(c.parent,c.model,c.inverse,c.logical,c.height,Stage().Allows(c.actor.id,GetTickCount64()),shifted,c.staged)){
    if(c.height>0){const auto now=GetTickCount64();auto due=nextSkipReport.load();if(now>=due && nextSkipReport.compare_exchange_strong(due,now+250))spdlog::info("POSE_SKIP actor={:08x} requested_cm={:.3f} logical_xyz={:.5f},{:.5f},{:.5f} parent_xyz={:.5f},{:.5f},{:.5f} model_xyz={:.5f},{:.5f},{:.5f}",c.actor.id,c.height*100,c.logical[0],c.logical[1],c.logical[2],c.parent[3],c.parent[7],c.parent[11],c.model[3],c.model[7],c.model[11]);}
    // Preserve native-pose observation with zero applied height even when shifting is refused.
   }
   else if(c.height>0){useParent=shifted.parent.data();useModel=shifted.model.data();useInverse=shifted.inverse.data();}
  }
 }catch(...){c={};}
 HeadCapture capture{c.actor,{}};
 const auto stage=Stage();HeadCaptureScope headScope(root?(c.creature && stage.mode && stage.scripted && !stage.ending && GetTickCount64()<stage.expires?&capture:nullptr):headCapture);
 heels::Frame weaponFrame{};
 try{if(WeaponScaleNodeFrame(self,useParent,useModel,useInverse,weaponFrame)){useParent=weaponFrame.parent.data();}}catch(...){}
 ++depth;nodeOriginal(self,edx,useParent,useModel,useInverse);--depth;
 try{CaptureComputedHead(self);}catch(...){}
 if(c.creature)try{
  const bool applied=useParent!=parent;Check(self,c,applied);
  ReportHelmetSnapshot("world",c.creature,c.creature);
  if(capture.world){auto point=*capture.world;point[2]-=applied?c.height:0.f;
   std::lock_guard guard(mutex);if(headSamples.size()>=1024 && !headSamples.contains(c.actor.id))headSamples.clear();headSamples[c.actor.id]={c.actor,point,GetTickCount64()};}
 }catch(...){}

}
using DialogEndFn=std::uintptr_t(__fastcall*)(void*,int,std::int32_t);
using DialogBeginFn=std::uintptr_t(__fastcall*)(void*,int);
using RendererViewFn=std::uintptr_t(__fastcall*)(void*,int,const void*);
DialogEndFn dialogEndOriginal=nullptr;RendererViewFn rendererViewOriginal=nullptr;
DialogBeginFn dialogBeginOriginal=nullptr;
using RendererPoseFn=std::uintptr_t(__fastcall*)(void*,int,const void*,const void*);
using RendererReadFn=std::uintptr_t(__fastcall*)(void*,int,void*);
RendererPoseFn rendererPoseOriginal=nullptr;RendererReadFn rendererReadOriginal=nullptr;
std::atomic<DWORD> dialogueThread=0;
void RestoreMainCamera();
using DialogUpdateFn=void(__fastcall*)(void*,int,float);
using AutoCameraFn=void(__fastcall*)(void*,int);
using CameraPositionFn=void(__fastcall*)(void*,int,const void*,float,std::uint32_t);
using CameraLookFn=void(__fastcall*)(void*,int,std::uint32_t,const void*,const void*);
DialogUpdateFn dialogUpdateOriginal=nullptr;AutoCameraFn autoCameraOriginal=nullptr;
CameraPositionFn cameraPositionOriginal=nullptr;CameraLookFn cameraLookOriginal=nullptr;
std::atomic<ULONGLONG> nextDialogPath=0,nextAutoReport=0,nextPositionReport=0,nextLookReport=0;
bool Due(std::atomic<ULONGLONG>& next){const auto now=GetTickCount64();auto t=next.load();return now>=t && next.compare_exchange_strong(t,now+2000);}
void CaptureStage(void* self){
 std::uint8_t scripted=0;std::uint32_t focus=0xffffffff;
 if(!Read(reinterpret_cast<std::uintptr_t>(self)+0x20c,scripted) || !Read(reinterpret_cast<std::uintptr_t>(self)+0x16c,focus))return;
 const heels::DialogueState snapshot{reinterpret_cast<std::uintptr_t>(self),GetTickCount64()+1000,focus,HeroId(),scripted!=0};
 std::lock_guard guard(stageMutex);if(stageState.mode==snapshot.mode && stageState.ending)return;stageState=snapshot;
}
std::uintptr_t __fastcall DialogBegin(void* self,int edx){
 dialogueThread.store(GetCurrentThreadId());
 {std::lock_guard guard(stageMutex);stageState={};}
 const auto nativeResult=dialogBeginOriginal(self,edx);
 try{CaptureStage(self);}catch(...){}
 return nativeResult;
}
void __fastcall DialogUpdate(void* self,int edx,float dt){
 dialogueThread.store(GetCurrentThreadId());
 dialogueTransitionUntil.store(GetTickCount64()+1000);
 try{CaptureStage(self);const auto snapshot=Stage();if(Due(nextDialogPath))spdlog::info("DIALOG_PATH native_cutscene_flag={} focus_actor={:08x} hero_actor={:08x} compensation_enabled={}",snapshot.scripted,snapshot.focus,snapshot.hero,dialogueCompensation);}catch(...){}
 dialogUpdateOriginal(self,edx,dt);
 try{CaptureStage(self);}catch(...){}
}
std::uintptr_t __fastcall DialogEnd(void* self,int edx,std::int32_t result){
 {std::lock_guard guard(stageMutex);if(stageState.mode==reinterpret_cast<std::uintptr_t>(self))stageState.ending=true;}
 try{RestoreMainCamera();}catch(...){}
 const auto nativeResult=dialogEndOriginal(self,edx,result);
 {std::lock_guard guard(stageMutex);if(stageState.mode==reinterpret_cast<std::uintptr_t>(self)){stageState.ending=true;stageState.expires=GetTickCount64()+250;}}
 spdlog::info("DIALOG_END staged pose cleanup grace_ms=250; cinematic trace stopped");return nativeResult;
}
// Capture native baselines at both view setters; reconcile only the live scene camera.
struct ViewState {heels::Matrix native{};float applied=0;std::uint32_t target=0xffffffff;std::uint64_t serial=0;};
std::mutex viewMutex;std::unordered_map<std::uintptr_t,ViewState> views;
thread_local unsigned cameraSyncDepth=0;
std::atomic<ULONGLONG> nextViewReport=0;
std::uintptr_t MainCamera(){
 if(dialogueThread.load()!=GetCurrentThreadId())return 0;
 std::uintptr_t graphics=0,vt=0,getter=0,wrapper=0,camera=0,cameraVt=0;
 if(!Read(dao::Module::Address(dao::AddressId{100485}),graphics) || !graphics || !Read(graphics,vt) || !Read(vt+9*4,getter))return 0;
 const auto base=dao::Module::Base();MEMORY_BASIC_INFORMATION page{};
 if(!base || getter<base || getter>=base+0xa00000 || !VirtualQuery(reinterpret_cast<void*>(getter),&page,sizeof(page)) || page.State!=MEM_COMMIT || (page.Protect&PAGE_GUARD) || !(page.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))return 0;
 using SceneFn=std::uintptr_t(__fastcall*)(void*,int);
 const auto scene=reinterpret_cast<SceneFn>(getter)(reinterpret_cast<void*>(graphics),0);
 if(!scene || !Read(scene+0xc0,wrapper) || !wrapper || !Read(wrapper+4,camera) || !camera || !Read(camera,cameraVt) || cameraVt!=dao::Module::Address(dao::AddressId{74603}))return 0;
 return camera;
}
void RememberView(void* self){
 const auto address=reinterpret_cast<std::uintptr_t>(self);std::uintptr_t vt=0;heels::Matrix view{},world{};
 if(!Read(address,vt) || vt!=dao::Module::Address(dao::AddressId{74603}) || !Read(address+0x80,view) || !heels::CameraWorld(view,world))return;
 std::lock_guard guard(viewMutex);if(views.size()>=64 && !views.contains(address)){auto victim=std::find_if(views.begin(),views.end(),[](const auto& item){return item.second.applied==0;});if(victim==views.end())return;views.erase(victim);}
 auto& state=views[address];state.native=view;state.applied=0;++state.serial;
}
std::uintptr_t __fastcall RendererView(void* self,int edx,const void* matrix){
 const auto result=rendererViewOriginal(self,edx,matrix);try{RememberView(self);}catch(...){}return result;
}
std::uintptr_t __fastcall RendererPose(void* self,int edx,const void* position,const void* quaternion){
 const auto result=rendererPoseOriginal(self,edx,position,quaternion);try{RememberView(self);}catch(...){}return result;
}
bool CameraSubjects(const heels::DialogueState& stage,std::array<heels::CameraSubject,2>& result,const char*& reason){
 const std::array ids{stage.focus,stage.hero};const auto now=GetTickCount64();
 for(unsigned i=0;i<2;++i){
  if(i && ids[i]==ids[0])continue;
  const auto ptr=Live(ids[i]);const auto actor=ptr?Identify(ptr):std::nullopt;if(!actor){reason="actor_unavailable";return false;}
  std::optional<std::array<float,4>> foot;float h=0;HeadSample head;
  {std::lock_guard guard(mutex);foot=actors.NativePosition(*actor,now);h=actors.RenderedHeight(*actor,now);
   const auto found=headSamples.find(ids[i]);if(found==headSamples.end() || found->second.actor!=*actor || now<found->second.at || now-found->second.at>1000){reason="computed_head_unavailable";return false;}head=found->second;}
  if(!foot){reason="native_pose_unavailable";return false;}
  const auto& point=head.native;
  const float dx=point[0]-(*foot)[0],dy=point[1]-(*foot)[1],dz=point[2]-(*foot)[2];
  if(!std::isfinite(dx) || !std::isfinite(dy) || !std::isfinite(dz) || dx*dx+dy*dy>4 || dz<0.1f || dz>3.5f){reason="computed_head_outside_body";return false;}
  result[i]={ids[i],point,h,true};
 }
 reason="subjects_available";return true;
}
void SyncCamera(std::uintptr_t camera,bool restore){
 ViewState state;heels::Matrix current{},world{};
 if(!Read(camera+0x80,current) || !heels::CameraWorld(current,world))return;
 {std::lock_guard guard(viewMutex);
  if(views.size()>=64 && !views.contains(camera)){auto victim=std::find_if(views.begin(),views.end(),[](const auto& item){return item.second.applied==0;});if(victim==views.end())return;views.erase(victim);}
  auto [it,inserted]=views.try_emplace(camera);auto& entry=it->second;
  if(inserted || !heels::NearMatrix(current,heels::RaisedView(entry.native,entry.applied))){entry.native=current;entry.applied=0;++entry.serial;}
  state=entry;
 }
 float height=0;heels::ShotChoice choice;const char* reason="inactive";std::array<heels::CameraSubject,2> subjects{};const auto stage=Stage();const auto now=GetTickCount64();
 if(!restore && dialogueCompensation && active.load() && stage.mode && stage.scripted && !stage.ending && now<stage.expires && heels::CameraWorld(state.native,world)){
  if(CameraSubjects(stage,subjects,reason)){choice=heels::ChooseShot(world,subjects,state.target);reason=choice.valid?"target_selected":"shot_ambiguous_or_outside_ray";}
  if(choice.valid)height=choice.height;
 }
 const auto corrected=heels::RaisedView(state.native,height);
 // Head lookup can refresh models. Do not overwrite an intervening native camera write.
 {std::lock_guard guard(viewMutex);const auto it=views.find(camera);if(it==views.end() || it->second.serial!=state.serial)return;}
 heels::Matrix latest{};if(!Read(camera+0x80,latest) || !heels::NearMatrix(current,latest))return;
 if(!heels::NearMatrix(current,corrected))rendererViewOriginal(reinterpret_cast<void*>(camera),0,corrected.data());
 {std::lock_guard guard(viewMutex);auto it=views.find(camera);if(it!=views.end() && it->second.serial==state.serial){it->second.applied=height;it->second.target=choice.valid?choice.id:0xffffffff;}}
 if((state.applied!=height || Due(nextViewReport)) && (stage.mode || state.applied>0)){
  spdlog::info("CINEMATIC_CAMERA camera={:08x} target={:08x} target_inferred={} held={} added_cm={:.3f} restored={} scripted={} reason={}",camera,choice.id,choice.valid,choice.held,height*100,restore,stage.scripted,reason);
  if(stage.mode && stage.scripted && !stage.ending && now<stage.expires){
   spdlog::info("CINEMATIC_INPUT camera_xyz={:.5f},{:.5f},{:.5f} forward_xyz={:.5f},{:.5f},{:.5f} focus={:08x} focus_valid={} focus_head={:.5f},{:.5f},{:.5f} focus_cm={:.3f} hero={:08x} hero_valid={} hero_head={:.5f},{:.5f},{:.5f} hero_cm={:.3f}",world[3],world[7],world[11],-world[2],-world[6],-world[10],stage.focus,subjects[0].valid,subjects[0].nativeHead[0],subjects[0].nativeHead[1],subjects[0].nativeHead[2],subjects[0].height*100,stage.hero,subjects[1].valid,subjects[1].nativeHead[0],subjects[1].nativeHead[1],subjects[1].nativeHead[2],subjects[1].height*100);
  }
 }
}
struct CameraSyncScope {CameraSyncScope(){++cameraSyncDepth;}~CameraSyncScope(){--cameraSyncDepth;}};
void RestoreMainCamera(){
 if(cameraSyncDepth)return;CameraSyncScope scope;const auto camera=MainCamera();if(camera)SyncCamera(camera,true);
}
std::uintptr_t __fastcall RendererRead(void* self,int edx,void* matrix){
 if(!cameraSyncDepth && dialogueThread.load()!=GetCurrentThreadId()){const auto stage=Stage();if(stage.mode && stage.scripted && !stage.ending && GetTickCount64()<stage.expires && Due(nextViewReport))spdlog::info("CINEMATIC_CAMERA skipped: view read off dialogue thread");}
 if(!cameraSyncDepth && dialogueThread.load()==GetCurrentThreadId())try{
  CameraSyncScope scope;const auto address=reinterpret_cast<std::uintptr_t>(self);
  if(MainCamera()==address)SyncCamera(address,false);
 }catch(...){}
 return rendererReadOriginal(self,edx,matrix);
}

void __fastcall AutoCamera(void* self,int edx){
 heels::CameraContext context;
 try{
  std::uint32_t id=0xffffffff;std::uintptr_t camera=0;
  if(Read(reinterpret_cast<std::uintptr_t>(self)+0x16c,id) && Read(reinterpret_cast<std::uintptr_t>(self)+0x58,camera) && camera){
   context.camera=camera;context.actor=id;
   if(dialogueCompensation && active.load()){
    const auto ptr=Live(id);const auto actor=ptr?Identify(ptr):std::nullopt;
    if(actor){std::lock_guard guard(mutex);context.height=actors.RenderedHeight(*actor,GetTickCount64());}
   }
   if(Due(nextAutoReport))spdlog::info("DIALOG_AUTO focus_actor={:08x} visual_height_cm={:.3f} camera={:08x}",id,context.height*100,camera);
  }
 }catch(...){context={};}
 heels::CameraScope scope(dialogueCamera,context);
 autoCameraOriginal(self,edx);
}
void __fastcall CameraPosition(void* self,int edx,const void* position,float duration,std::uint32_t mode){
 alignas(16) heels::CameraVector input{},shifted{};const void* value=position;
 try{
  if(dialogueCamera.camera==reinterpret_cast<std::uintptr_t>(self) && dialogueCamera.height>0 && Read(reinterpret_cast<std::uintptr_t>(position),input) && heels::Compensate(dialogueCamera,reinterpret_cast<std::uintptr_t>(self),input,shifted)){
   value=shifted.data();if(Due(nextPositionReport))spdlog::info("DIALOG_CAMERA_POSITION actor={:08x} added_cm={:.3f} native_z={:.5f} corrected_z={:.5f}",dialogueCamera.actor,dialogueCamera.height*100,input[2],shifted[2]);
  }
 }catch(...){value=position;}
 cameraPositionOriginal(self,edx,value,duration,mode);
}
void __fastcall CameraLook(void* self,int edx,std::uint32_t actor,const void* offset,const void* other){
 alignas(16) heels::CameraVector input{},shifted{};const void* value=offset;
 try{
  if(actor==dialogueCamera.actor && dialogueCamera.camera==reinterpret_cast<std::uintptr_t>(self) && dialogueCamera.height>0 && Read(reinterpret_cast<std::uintptr_t>(offset),input) && heels::Compensate(dialogueCamera,reinterpret_cast<std::uintptr_t>(self),input,shifted)){
   value=shifted.data();if(Due(nextLookReport))spdlog::info("DIALOG_CAMERA_LOOK actor={:08x} added_cm={:.3f} native_offset_z={:.5f} corrected_offset_z={:.5f}",actor,dialogueCamera.height*100,input[2],shifted[2]);
  }
 }catch(...){value=offset;}
 cameraLookOriginal(self,edx,actor,value,other);
}

}
extern "C" __declspec(dllexport) bool __cdecl DAOLib_Load(const dao::LoaderInfo* info){
 if(!info || info->apiVersion!=dao::kPluginApiVersion || info->structSize<sizeof(dao::LoaderInfo) || !info->pluginPath)return false;
 try{
 dao::OpenLog("equipment_appearance");spdlog::info("Equipment Appearance 0.2.1: post-filter helmet appearance restoration; register-preserving bridge; NO slot getter hooks");
 const auto ini=std::filesystem::path(info->pluginPath).parent_path()/L"equipment_appearance.ini";
 if(!GetPrivateProfileIntW(L"General",L"Enabled",0,ini.c_str())){spdlog::info("Disabled by INI");return true;}

 const auto plugins=std::filesystem::path(info->pluginPath).parent_path().parent_path();
 for(const auto& legacy:std::array{L"heels_probe",L"robe_fullbody"}){
  const auto dll=std::wstring(legacy)+L".dll";
  if(GetModuleHandleW(dll.c_str()) || std::filesystem::exists(plugins/legacy/dll)){spdlog::error("REFUSED: legacy heels_probe/robe_fullbody DLL present. Move both legacy plugin folders outside daolib/plugins before using Equipment Appearance.");return false;}
 }
 LoadHiddenHelmets(ini);
 LoadWeaponProbeConfig(ini);
 showHelmetDialog=GetPrivateProfileIntW(L"Helmets",L"ShowInDialogues",1,ini.c_str())!=0;
 spdlog::info("Dialogue helmets {}; runtime quickbar visibility available; ear hiding not implemented",showHelmetDialog?"ON (experimental)":"native behavior");
 helmetDiagnostics=GetPrivateProfileIntW(L"HelmetDiagnostics",L"Enabled",1,ini.c_str())!=0;
 active.store(GetPrivateProfileIntW(L"Heels",L"Enabled",1,ini.c_str())!=0);
 heels::fullBodyEnabled=GetPrivateProfileIntW(L"FullBody",L"Enabled",1,ini.c_str())!=0;
 ea_body::getBase=reinterpret_cast<ea_body::BaseFn>(dao::Module::Address(dao::AddressId{17750}));
 ea_body::getVariation=reinterpret_cast<ea_body::VariationFn>(dao::Module::Address(dao::AddressId{17752}));
 spdlog::info("Features: heels={} full_body={} dialogue_helmets={} quickbar_toggle=true",active.load(),heels::fullBodyEnabled,showHelmetDialog);
 inventoryPreview=GetPrivateProfileIntW(L"Preview",L"InventoryLift",0,ini.c_str())!=0;
 spdlog::info("Inventory preview lift {}; party-picker lift enabled",inventoryPreview?"ON (experimental)":"OFF (native position)");
 dialogueCompensation=GetPrivateProfileIntW(L"Camera",L"DialogueCompensation",1,ini.c_str())!=0;
 if(!info->knownExecutable || !Verify() || !VerifyGuiDisk(info->executablePath) || (weaponConfig.ProbeEnabled() && !VerifyWeaponProbeDisk(info->executablePath))){spdlog::error("REFUSED: unverified executable/layout");return false;}
 const auto init=MH_Initialize();if(init!=MH_OK && init!=MH_ERROR_ALREADY_INITIALIZED)return false;
 struct Hook {unsigned id;void* replacement;void** original;};
 const std::array<Hook,22> hooks{{
  {14437,reinterpret_cast<void*>(&WeaponLoadProbe),reinterpret_cast<void**>(&EA_WeaponLoadOriginal)},
  {14439,reinterpret_cast<void*>(&WeaponAttachProbe),reinterpret_cast<void**>(&EA_WeaponAttachOriginal)},
  {16032,reinterpret_cast<void*>(&SavedHelmetInt),reinterpret_cast<void**>(&EA_SavedIntOriginal)},
  {18309,reinterpret_cast<void*>(&PartyCreate),reinterpret_cast<void**>(&partyCreateOriginal)},
  {14965,reinterpret_cast<void*>(&HelmetBuildProbe),reinterpret_cast<void**>(&EA_BuildOriginal)},
  {20638,reinterpret_cast<void*>(&HelmetSceneMask),reinterpret_cast<void**>(&EA_MaskOriginal)},
  {17835,reinterpret_cast<void*>(&ea_body::Body),reinterpret_cast<void**>(&ea_body::originalBody)},
  {17767,reinterpret_cast<void*>(&ea_body::Clothing),reinterpret_cast<void**>(&ea_body::originalClothing)},
  {20296,reinterpret_cast<void*>(&PreviewCreate),reinterpret_cast<void**>(&previewCreateOriginal)},
  {20120,reinterpret_cast<void*>(&PaperTick),reinterpret_cast<void**>(&paperTickOriginal)},
  {18290,reinterpret_cast<void*>(&PartyTick),reinterpret_cast<void**>(&partyTickOriginal)},
  {14160,reinterpret_cast<void*>(&Move),reinterpret_cast<void**>(&moveOriginal)},
  {8348,reinterpret_cast<void*>(&Node),reinterpret_cast<void**>(&nodeOriginal)},
  {13897,reinterpret_cast<void*>(&DialogUpdate),reinterpret_cast<void**>(&dialogUpdateOriginal)},
  {13891,reinterpret_cast<void*>(&AutoCamera),reinterpret_cast<void**>(&autoCameraOriginal)},
  {15033,reinterpret_cast<void*>(&CameraPosition),reinterpret_cast<void**>(&cameraPositionOriginal)},
  {15035,reinterpret_cast<void*>(&CameraLook),reinterpret_cast<void**>(&cameraLookOriginal)},
  {13951,reinterpret_cast<void*>(&DialogEnd),reinterpret_cast<void**>(&dialogEndOriginal)},
  {2724,reinterpret_cast<void*>(&RendererView),reinterpret_cast<void**>(&rendererViewOriginal)},
  {2723,reinterpret_cast<void*>(&RendererPose),reinterpret_cast<void**>(&rendererPoseOriginal)},
  {2729,reinterpret_cast<void*>(&RendererRead),reinterpret_cast<void**>(&rendererReadOriginal)},
  {13932,reinterpret_cast<void*>(&DialogBegin),reinterpret_cast<void**>(&dialogBeginOriginal)}
 }};
 std::vector<void*> created;const auto rollback=[&](){for(auto* p:created){MH_DisableHook(p);MH_RemoveHook(p);}};
 for(const auto& h:hooks){if((h.id==14437 || h.id==14439) && !weaponConfig.ProbeEnabled())continue;if(!heels::fullBodyEnabled && (h.id==17835 || h.id==17767))continue;if(!inventoryPreview && (h.id==20296 || (h.id==20120 && !helmetDiagnostics)))continue;auto* p=reinterpret_cast<void*>(dao::Module::Address(dao::AddressId{h.id}));if(!p || MH_CreateHook(p,h.replacement,h.original)!=MH_OK){rollback();spdlog::error("REFUSED: camera/model hook creation failed, ID {}",h.id);return false;}created.push_back(p);if(h.id==14437 || h.id==14439)spdlog::info("WEAPON_PROBE_HOOK revision=11 id={} address={:08x}",h.id,reinterpret_cast<std::uintptr_t>(p));}
 for(auto* p:created)if(MH_EnableHook(p)!=MH_OK){rollback();return false;}
 InstallHelmetQuickbar(ini.parent_path());
 if(weaponConfig.ProbeEnabled())dao::OnFrame([]{WeaponProbeFrame();});
 spdlog::info("Ready: talk to a character wearing HEELS. Automatic camera position and look target receive that actor's recent visual height only within the native auto-dialogue calculation. Scripted correction uses the active scene camera only; shot target inferred from native head alignment. Ambiguous shots are left unchanged.");return true;
 }catch(...){return false;}
}
