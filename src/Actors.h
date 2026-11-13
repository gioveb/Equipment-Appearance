#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <optional>
#include <algorithm>
#include <array>
namespace heels {
struct Actor {
 std::uint32_t id=0xffffffff,area=0xffffffff;
 std::uintptr_t creature=0,model=0;
 bool operator==(const Actor&)const=default;
};
class Actors {
 struct Entry{Actor actor;std::uint64_t seen=0;bool blocked=false;float renderedHeight=0;std::uint64_t renderedAt=0;std::array<float,4> nativePosition{};};
 std::unordered_map<std::uint32_t,Entry> entries;
 std::unordered_map<std::uintptr_t,std::uint32_t> models;
 std::size_t limit;
 void Remove(std::unordered_map<std::uint32_t,Entry>::iterator it){
  auto m=models.find(it->second.actor.model);if(m!=models.end() && m->second==it->first)models.erase(m);entries.erase(it);
 }
 public:
 explicit Actors(std::size_t capacity=1024):limit(capacity){}
 bool Upsert(Actor actor,std::uint64_t now){
  if(!actor.creature || !actor.model || actor.id==0xffffffff || !limit)return false;
  auto it=entries.find(actor.id);
  const bool changed=it==entries.end() || it->second.actor!=actor;
  if(it!=entries.end() && !changed){it->second.seen=now;return false;}
  if(it!=entries.end())Remove(it);
  auto conflict=models.find(actor.model);
  if(conflict!=models.end()){auto other=entries.find(conflict->second);if(other!=entries.end())Remove(other);}
  if(entries.size()>=limit)Remove(std::min_element(entries.begin(),entries.end(),[](const auto&a,const auto&b){return a.second.seen<b.second.seen;}));
  entries[actor.id]={actor,now,false};models[actor.model]=actor.id;return true;
 }
 std::optional<Actor> Find(std::uintptr_t model)const{
  const auto m=models.find(model);if(m==models.end())return {};
  const auto e=entries.find(m->second);if(e==entries.end() || e->second.blocked)return {};return e->second.actor;
 }
 void Erase(const Actor& expected){const auto it=entries.find(expected.id);if(it!=entries.end() && it->second.actor==expected)Remove(it);}
 void Block(const Actor& expected){const auto it=entries.find(expected.id);if(it!=entries.end() && it->second.actor==expected)it->second.blocked=true;}
 void Rendered(const Actor& expected,float height,std::uint64_t now,const std::array<float,4>& position={}){const auto it=entries.find(expected.id);if(it!=entries.end() && it->second.actor==expected && !it->second.blocked){it->second.renderedHeight=height;it->second.renderedAt=now;it->second.nativePosition=position;}}
 std::optional<std::array<float,4>> NativePosition(const Actor& expected,std::uint64_t now)const{
  const auto it=entries.find(expected.id);if(it==entries.end() || it->second.actor!=expected || it->second.blocked || !it->second.renderedAt || now<it->second.renderedAt || now-it->second.renderedAt>1000)return {};return it->second.nativePosition;
 }
 float RenderedHeight(const Actor& expected,std::uint64_t now)const{
  const auto it=entries.find(expected.id);if(it==entries.end() || it->second.actor!=expected || it->second.blocked || !it->second.renderedAt || now<it->second.renderedAt || now-it->second.renderedAt>1000)return 0;
  return it->second.renderedHeight;
 }
 std::vector<Actor> Snapshot()const{std::vector<Actor> out;out.reserve(entries.size());for(const auto&[id,e]:entries)out.push_back(e.actor);return out;}
 std::vector<Actor> RaisedCandidates()const{std::vector<Actor> out;for(const auto&[id,e]:entries)if(!e.blocked && e.renderedHeight>0)out.push_back(e.actor);return out;}
 std::size_t Size()const{return entries.size();}
};
}
