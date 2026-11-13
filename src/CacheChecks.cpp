#include "EquipmentHeight.h"
#include <iostream>
#include <stdexcept>
namespace {
unsigned checks=0,calls=0;void Check(bool b){++checks;if(!b)throw std::runtime_error("actor height-cache check failed");}
template<class T>void Store(std::uintptr_t at,const T& v){std::memcpy(reinterpret_cast<void*>(at),&v,sizeof(v));}
std::uintptr_t Ptr(const void* p){return reinterpret_cast<std::uintptr_t>(p);}
struct Fixture {
 std::array<std::byte,0x1000> creature{};std::array<std::byte,0x200> appearance{};
 std::array<std::byte,0x100> equipment{};std::array<std::byte,0x800> boots{};iv::IntegerVariable value{};
 heels::Actor actor{};
 void Init(unsigned id,float cm){
  actor={id,1,Ptr(creature.data()),Ptr(creature.data())+8};
  Store(actor.creature+0x10,id);Store(actor.creature+0x7b0,Ptr(appearance.data()));Store(Ptr(appearance.data())+0x58,std::uint16_t{1});
  Store(Ptr(appearance.data())+0x5a,std::uint16_t{0xa55a}); // Regression: adjacent flags are NOT part of appearance ID.
  Store(actor.creature+0xbec,Ptr(equipment.data()));Store(Ptr(equipment.data()),std::uintptr_t{0x1111});Store(Ptr(equipment.data())+0x24,std::uintptr_t{0x2222});
  Store(Ptr(boots.data()),std::uintptr_t{0x3333});Store(Ptr(boots.data())+0x4a8,id);
  value={0,0,2,std::bit_cast<std::int32_t>(cm),0};
  const auto p=static_cast<unsigned>(Ptr(&value));Store(Ptr(boots.data())+0x300,iv::NativeVector{0,p,p+20,p+20});
 }
};
Fixture a,b,reused;
iv::NativeVector pool{};std::uint32_t namePtr=0;std::array<std::byte,64> nameNode{};
std::uintptr_t __fastcall EquippedMock(void* equipment,int,const void* data){
 ++calls;const auto* key=static_cast<const std::int32_t*>(data);if(key[2]!=6)return 0;
 for(auto* f:{&a,&b,&reused})if(equipment==f->equipment.data())return Ptr(f->boots.data());return 0;
}
}
namespace dao {
bool TryCopy(void* out,std::uintptr_t at,std::size_t n)noexcept{SIZE_T done=0;return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(at),out,n,&done) && done==n;}
namespace Module {std::uintptr_t Address(AddressId id){switch(static_cast<unsigned>(id)){case 100262:return Ptr(&pool);case 68551:return 0x3333;case 68885:return 0x1111;case 68886:return 0x2222;case 19468:return Ptr(reinterpret_cast<const void*>(&EquippedMock));default:return 0;}}}
namespace gda {
std::optional<std::int32_t> GetInt(std::int32_t,std::int32_t,std::uint32_t){return 100;}
std::optional<std::wstring> GetString(std::int32_t table,std::int32_t row,std::uint32_t){if(table==2)return row==1?L"p":L"h";return L"rob";}
}}
int main(){try{
 spdlog::set_level(spdlog::level::off);
 dao::core::WStringRep name;std::wcscpy(name.data.buffer,L"HEELS");name.size=5;Store(Ptr(nameNode.data())+12,name);namePtr=static_cast<unsigned>(Ptr(nameNode.data()));const auto begin=static_cast<unsigned>(Ptr(&namePtr));pool={0,begin,begin+4,begin+4};
 a.Init(9001,5);b.Init(9002,8);reused.Init(9001,12);
 Check(std::abs(heels::HeightFor(a.actor,1000)-0.05f)<0.00001f);Check(std::abs(heels::HeightFor(b.actor,1000)-0.08f)<0.00001f);Check(calls==4);
 for(unsigned i=0;i<100;++i){Check(std::abs(heels::HeightFor(a.actor,1100)-0.05f)<0.00001f);Check(std::abs(heels::HeightFor(b.actor,1100)-0.08f)<0.00001f);}Check(calls==4);
 a.value.value=std::bit_cast<std::int32_t>(6.f);
 Check(std::abs(heels::HeightFor(a.actor,1199)-0.05f)<0.00001f);Check(calls==4);
 Check(std::abs(heels::HeightFor(a.actor,1200)-0.06f)<0.00001f);Check(calls==6);
 Check(std::abs(heels::HeightFor(b.actor,1200)-0.08f)<0.00001f);Check(calls==8);
 Store(Ptr(a.appearance.data())+0x58,std::uint16_t{2});Check(heels::HeightFor(a.actor,1201)==0);Check(calls==8);
 Store(Ptr(a.appearance.data())+0x58,std::uint16_t{1});Check(std::abs(heels::HeightFor(a.actor,1202)-0.06f)<0.00001f);Check(calls==10);
 Check(std::abs(heels::HeightFor(reused.actor,1203)-0.12f)<0.00001f);Check(calls==12);
 Check(std::abs(heels::HeightFor(b.actor,1204)-0.08f)<0.00001f);Check(calls==12);
 reused.actor.area=2;Check(std::abs(heels::HeightFor(reused.actor,1205)-0.12f)<0.00001f);Check(calls==14);
 std::cout<<"PASS: "<<checks<<" production height-cache checks; alternating actors, bounded polling, value updates, shapeshift, ID/address replacement and area change\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
