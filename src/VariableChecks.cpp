#include "EquipmentHeight.h"
#include <iostream>
#include <stdexcept>
#include <limits>
namespace {
unsigned checks=0;void Check(bool b){++checks;if(!b)throw std::runtime_error("variable/policy check failed");}
std::array<std::byte,0x800> item{};iv::NativeVector pool{};
std::array<std::uint32_t,2> names{};
std::array<std::byte,64> name0{},name1{};
std::array<std::vector<iv::IntegerVariable>,6> values;
std::uintptr_t Address(const void* p){return reinterpret_cast<std::uintptr_t>(p);}
template<class T>void Store(std::uintptr_t at,const T& v){std::memcpy(reinterpret_cast<void*>(at),&v,sizeof(v));}
void SetName(std::array<std::byte,64>& node,const wchar_t* text){dao::core::WStringRep s;std::wcscpy(s.data.buffer,text);s.size=static_cast<std::uint32_t>(std::wcslen(text));Store(Address(node.data())+12,s);}
void Reset(){item.fill(std::byte{});for(auto& v:values)v.clear();Store(Address(item.data()),std::uintptr_t{0x12345678});}
void Add(unsigned type,float value,unsigned index=0){values[type].push_back({0,index,type,std::bit_cast<std::int32_t>(value),0});}
heels::Value Read(){
 for(unsigned t=1;t<=5;++t){const auto begin=values[t].empty()?0:Address(values[t].data());const auto end=begin+values[t].size()*20;Store(Address(item.data())+0x2e0+t*16,iv::NativeVector{0,static_cast<unsigned>(begin),static_cast<unsigned>(end),static_cast<unsigned>(end)});}
 return heels::ReadHeels(Address(item.data()));
}
}
namespace dao {
bool TryCopy(void* out,std::uintptr_t at,std::size_t n)noexcept{SIZE_T done=0;return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void*>(at),out,n,&done) && done==n;}
namespace Module {std::uintptr_t Address(AddressId id){if(static_cast<unsigned>(id)==100262)return ::Address(&pool);if(static_cast<unsigned>(id)==68551)return 0x12345678;return 0;}}
}
int main(){try{
 SetName(name0,L"HEELS");SetName(name1,L"OTHER");names={static_cast<unsigned>(Address(name0.data())),static_cast<unsigned>(Address(name1.data()))};
 pool={0,static_cast<unsigned>(Address(names.data())),static_cast<unsigned>(Address(names.data())+8),static_cast<unsigned>(Address(names.data())+8)};
 Reset();Check(Read().status==heels::VariableStatus::Missing);
 for(float cm:{0.f,0.01f,0.5f,5.f,8.25f,20.f}){Reset();Add(2,cm);const auto v=Read();Check(v.status==heels::VariableStatus::Valid && v.cm==cm);}
 for(float cm:{-0.01f,20.01f,std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}){Reset();Add(2,cm);Check(Read().status==heels::VariableStatus::Invalid);}
 for(unsigned type:{1u,3u,4u,5u}){Reset();Add(type,5);Check(Read().status==heels::VariableStatus::Invalid);}
 Reset();Add(2,5);Add(2,5);Check(Read().status==heels::VariableStatus::Invalid);
 Reset();Add(2,5);Add(1,5);Check(Read().status==heels::VariableStatus::Invalid);
 Reset();Add(2,5);Add(1,8,1);Check(Read().status==heels::VariableStatus::Valid);
 Check(heels::ReadHeels(1).status==heels::VariableStatus::Invalid);
 Check(heels::ReadHeels(0).status==heels::VariableStatus::Missing);
 const heels::Value v5{heels::VariableStatus::Valid,5},v8{heels::VariableStatus::Valid,8},v0{heels::VariableStatus::Valid,0},bad{heels::VariableStatus::Invalid,0};
 Check(heels::SelectHeight(true,v8,true,v5)==8);Check(heels::SelectHeight(false,v8,true,v5)==5);
 Check(heels::SelectHeight(true,v0,true,v5)==5);Check(heels::SelectHeight(true,v0,false,v5)==0);
 Check(heels::SelectHeight(false,bad,true,bad)==0);Check(heels::SelectHeight(true,v8,false,v5)==8);
 for(unsigned base=0;base<100;++base)for(bool rob:{false,true})for(bool enabled:{false,true}){
  heels::Chest c{true,rob,static_cast<std::uint16_t>(base),1,1};
  const bool expected=base!=22 && base!=38 && !(base>=9 && base<=12 && rob && enabled);
  Check(heels::BootsVisible(1,c,enabled)==expected);
 }
 Check(heels::BootsVisible(0,{},true));Check(!heels::BootsVisible(1,{},false));
 std::cout<<"PASS: "<<checks<<" real-memory float reader / duplicates / invalid types / precedence / boot-visibility checks\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
