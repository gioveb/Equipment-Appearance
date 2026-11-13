#include "dao/Gui.h"
#include "dao/Frame.h"
#include "dao/core/GfxMovie.h"
#include <spdlog/spdlog.h>
#include <map>
#include <set>
#include <iostream>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <filesystem>
void InstallHelmetQuickbar(const std::filesystem::path&);
namespace {unsigned checks=0,controlled=130,rebuilds=0;bool head=true,rebuildOK=true,saveOK=true;std::wstring ownerName=L"Quickbar";std::set<unsigned> hidden;dao::gui::Value answer;std::function<void()> frame;dao::gui::MoviePatch patch;std::map<std::string,dao::gui::ExternalInterfaceHandler> calls;
void Check(bool ok){++checks;if(!ok)throw std::runtime_error("quickbar check failed "+std::to_string(checks));}
void Query(){calls.at("EA_HelmetState")({reinterpret_cast<void*>(1),"EA_HelmetState",{}});}
bool Click(unsigned id,void* owner=reinterpret_cast<void*>(1)){
 const dao::gui::Value arg=dao::gui::Value::Number(id);calls.at("EA_ToggleHelmet")({owner,"EA_ToggleHelmet",std::span{&arg,1}});return answer.AsBool().value_or(false);
}}
namespace dao {void OnFrame(std::function<void()> fn){frame=std::move(fn);}}
namespace dao::gui {
void OnMovieLoad(std::wstring_view name,MoviePatch fn){Check(name==L"quickbar");patch=std::move(fn);}
void OnExternalInterface(std::string name,ExternalInterfaceHandler fn){calls[name]=std::move(fn);}
void OnFSCommand(std::string,FSCommandHandler){}
std::wstring_view NameOf(const void* owner){return owner==reinterpret_cast<void*>(1)?std::wstring_view(ownerName):std::wstring_view(L"inventory");}
void* MovieOf(const void* owner){return const_cast<void*>(owner);}
std::optional<Movie::Answer> Movie::Get(const char* path)const{
 if(view_==reinterpret_cast<void*>(3) && std::string_view(path)=="_root.EA_UIIdentity"){Movie::Answer a;a.type=Value::Type::String;a.text="equipment_appearance:helmet_button:v1";return a;}return std::nullopt;
}
void Call::Return(const Value& value)const{answer=value;}
}
extern "C" bool __cdecl EA_HelmetHiddenV1(unsigned id){return hidden.contains(id);}
extern "C" void __cdecl EA_SetHelmetHiddenV1(unsigned id,bool value){if(value)hidden.insert(id);else hidden.erase(id);}
extern "C" unsigned __cdecl EA_ControlledV1(){return controlled;}
extern "C" bool __cdecl EA_HasHeadV1(unsigned){return head;}
extern "C" bool __cdecl EA_RebuildHelmetV1(unsigned){++rebuilds;return rebuildOK;}
extern "C" bool __cdecl EA_SaveHelmetPreferenceV1(unsigned,bool){return saveOK;}
extern "C" void __cdecl EA_UpdatePartyHelmetsV1(){}
extern "C" void __cdecl EA_RefreshSavedHelmetsV1(){}
int main(int argc,char** argv){try{
 spdlog::set_level(spdlog::level::off);Check(argc==2);InstallHelmetQuickbar({});Check(bool(frame) && bool(patch));
 for(auto name:{L"Quickbar",L"quickbar",L"QuickBar.GFX",L"ui/Quickbar.swf",L"GUI_QUICKBAR"}){ownerName=name;Query();Check(answer.AsNumber()==521);}
 calls.at("EA_HelmetState")({reinterpret_cast<void*>(3),"EA_HelmetState",{}});Check(answer.AsNumber()==521);
 calls.at("EA_HelmetState")({reinterpret_cast<void*>(2),"EA_HelmetState",{}});Check(answer.AsNumber()==-1);
 Query();Check(answer.AsNumber()==521);Check(!Click(131));Check(!Click(130,reinterpret_cast<void*>(2)));
 Check(Click(130));Check(!hidden.contains(130) && rebuilds==0);Check(!Click(130));frame();Check(hidden.contains(130) && rebuilds==1);Query();Check(answer.AsNumber()==522);
 controlled=100;Query();Check(answer.AsNumber()==401);Check(Click(100));controlled=130;frame();Check(!hidden.contains(100) && rebuilds==1);
 Check(Click(130));frame();Check(hidden.empty() && rebuilds==2);
 saveOK=false;Check(Click(130));frame();Check(hidden.empty() && rebuilds==2);saveOK=true;
 rebuildOK=false;Check(Click(130));frame();Check(hidden.empty() && rebuilds==4);rebuildOK=true;
 head=false;Query();Check(answer.AsNumber()==520);Check(!Click(130));head=true;
 controlled=0xffffffff;Query();Check(answer.AsNumber()==-1);Check(!Click(controlled));controlled=130;
 for(unsigned n=0;n<1000;++n){Check(Click(130));frame();Check(hidden.contains(130));controlled=100;Query();Check(answer.AsNumber()==401);controlled=130;Check(Click(130));frame();Check(hidden.empty());}
 std::ifstream input(argv[1],std::ios::binary);std::vector<char> raw((std::istreambuf_iterator<char>(input)),{});auto original=dao::core::gfx::Unpack(std::as_bytes(std::span{raw}));Check(original.has_value());auto changed=patch(*original);Check(changed && changed->size()>original->size());Check(changed->at(0)==original->at(0));
 std::cout<<"PASS: "<<checks<<" quickbar checks; selected-actor isolation, queued update, stale click rejection, rollback, unavailable actor/head and additive movie patch\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

