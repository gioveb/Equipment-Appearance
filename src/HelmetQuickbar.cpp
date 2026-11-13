// SPDX-License-Identifier: GPL-3.0-or-later
#include "dao/Gui.h"
#include "dao/Frame.h"
#include "dao/Module.h"
#include "dao/core/GfxMovie.h"
#include "dao/Log.h"
#include "HelmetUIActions.h"
#include <atomic>
#include <cwctype>
#include <fstream>
#include <filesystem>
#include <mutex>
extern "C" bool __cdecl EA_HelmetHiddenV1(unsigned);
extern "C" void __cdecl EA_SetHelmetHiddenV1(unsigned,bool);
extern "C" unsigned __cdecl EA_ControlledV1();
extern "C" bool __cdecl EA_HasHeadV1(unsigned);
extern "C" bool __cdecl EA_RebuildHelmetV1(unsigned);
extern "C" bool __cdecl EA_SaveHelmetPreferenceV1(unsigned,bool);
extern "C" void __cdecl EA_UpdatePartyHelmetsV1();
extern "C" void __cdecl EA_RefreshSavedHelmetsV1();
namespace {
std::atomic<unsigned> pending{0xffffffff};
void ApplyQueuedToggle(){
 EA_RefreshSavedHelmetsV1();
 EA_UpdatePartyHelmetsV1();
 const auto id=pending.exchange(0xffffffff);if(id==0xffffffff)return;
 if(EA_ControlledV1()!=id || !EA_HasHeadV1(id)){spdlog::info("HELMET_UI_CLICK rejected stale actor={:08x}",id);return;}
 const bool before=EA_HelmetHiddenV1(id);if(!EA_SaveHelmetPreferenceV1(id,!before)){spdlog::warn("HELMET_UI_TOGGLE preference write failed actor={:08x}",id);return;}EA_SetHelmetHiddenV1(id,!before);
 if(!EA_RebuildHelmetV1(id)){EA_SaveHelmetPreferenceV1(id,before);EA_SetHelmetHiddenV1(id,before);EA_RebuildHelmetV1(id);spdlog::warn("HELMET_UI_TOGGLE rollback actor={:08x}; visual rebuild rejected",id);return;}
 spdlog::info("HELMET_UI_TOGGLE actor={:08x} visible={} equipment_unchanged=true thread={}",id,before,GetCurrentThreadId());
}
bool QuickbarOwner(void* owner){
 if(!owner)return false;std::wstring name(dao::gui::NameOf(owner));
 const auto slash=name.find_last_of(L"/\\");if(slash!=std::wstring::npos)name.erase(0,slash+1);
 for(auto& c:name)c=static_cast<wchar_t>(std::towlower(c));
 if(name==L"quickbar" || name==L"quickbar.gfx" || name==L"quickbar.swf" || name==L"gui_quickbar")return true;
 // Some UI records have a scene name rather than the resource basename.
 // Accept only the explicit marker added by this plugin's quickbar patch.
 const auto view=dao::gui::MovieOf(owner);if(!view)return false;
 const auto marker=dao::gui::Movie{view}.Get("_root.EA_UIIdentity");
 return marker && marker->type==dao::gui::Value::Type::String && marker->text=="equipment_appearance:helmet_button:v1";
}
}
void InstallHelmetQuickbar(const std::filesystem::path&){
 dao::gui::OnMovieLoad(L"quickbar",[](std::span<const std::byte> movie){
  const auto bytes=std::as_bytes(std::span{helmetUIActions});auto patched=dao::core::gfx::AppendDoAction(movie,bytes);
  spdlog::info("HELMET_UI_PATCH success={} original_bytes={} action_bytes={}",patched.has_value(),movie.size(),bytes.size());return patched;
 });
 dao::gui::OnExternalInterface("EA_HelmetState",[](const dao::gui::Call& call){
  const bool accepted=QuickbarOwner(call.owner);static std::atomic<unsigned> reports=0;
  if(reports.fetch_add(1)<8){const auto name=dao::gui::NameOf(call.owner);spdlog::info("HELMET_UI_QUERY owner={} accepted={} controlled={:08x}",std::string(name.begin(),name.end()),accepted,EA_ControlledV1());}
  if(!accepted){call.Return(dao::gui::Value::Number(-1));return;}
  const auto id=EA_ControlledV1();if(id==0xffffffff){call.Return(dao::gui::Value::Number(-1));return;}
  const unsigned state=EA_HasHeadV1(id)?(EA_HelmetHiddenV1(id)?2:1):0;
  call.Return(dao::gui::Value::Number(static_cast<double>(id)*4+state));
 });
 dao::gui::OnExternalInterface("EA_ToggleHelmet",[](const dao::gui::Call& call){
  bool queued=false;if(QuickbarOwner(call.owner) && call.args.size()==1){
   const auto n=call.args[0].AsNumber();const auto id=EA_ControlledV1();
   if(n && id!=0xffffffff && *n==static_cast<double>(id) && EA_HasHeadV1(id)){unsigned empty=0xffffffff;queued=pending.compare_exchange_strong(empty,id);}
  }const auto name=dao::gui::NameOf(call.owner);spdlog::info("HELMET_UI_CLICK owner={} queued={} controlled={:08x} args={}",std::string(name.begin(),name.end()),queued,EA_ControlledV1(),call.args.size());call.Return(dao::gui::Value::Bool(queued));
 });
 dao::gui::OnFSCommand("EA_HelmetUIReady",[](void* owner,std::string_view){const auto name=dao::gui::NameOf(owner);spdlog::info("HELMET_UI_READY owner={} accepted={} eye-only button next to name; floating tooltip; saved character preference",std::string(name.begin(),name.end()),QuickbarOwner(owner));});
 dao::gui::OnFSCommand("EA_HelmetUIClickRejected",[](void* owner,std::string_view args){const auto name=dao::gui::NameOf(owner);spdlog::info("HELMET_UI_CLICK_REJECTED owner={} reason={}",std::string(name.begin(),name.end()),args);});
 dao::gui::OnFSCommand("EA_HelmetUIPointer",[](void* owner,std::string_view event){const auto name=dao::gui::NameOf(owner);spdlog::info("HELMET_UI_POINTER owner={} event={}",std::string(name.begin(),name.end()),event);});
 dao::gui::OnFSCommand("EA_HelmetUITooltip",[](void* owner,std::string_view event){const auto name=dao::gui::NameOf(owner);spdlog::info("HELMET_UI_TOOLTIP owner={} event={}",std::string(name.begin(),name.end()),event);});
 dao::OnFrame(&ApplyQueuedToggle);
 spdlog::info("HELMET_UI_REGISTERED quickbar eye button; queued game-thread updates; native per-character saved preferences");
}
