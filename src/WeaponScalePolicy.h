// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "third_party/json.hpp"
#include "DarkspawnMeshes.h"
#include <cmath>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <stdexcept>

namespace weapon_scale {
inline bool ValidScale(double n){return std::isfinite(n) && n>=0.05 && n<=10.0;}
inline std::string ResourceKey(std::string name){
 const auto slash=name.find_last_of("/\\");if(slash!=std::string::npos)name.erase(0,slash+1);
 for(auto& c:name)if(c>='A' && c<='Z')c=static_cast<char>(c-'A'+'a');return name;
}
inline bool IsDarkspawn(std::string_view resource){const auto key=ResourceKey(std::string(resource));return std::find(DarkspawnMeshes.begin(),DarkspawnMeshes.end(),key)!=DarkspawnMeshes.end();}
inline bool Hash(std::string_view s){return s.size()==64 && std::all_of(s.begin(),s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});}
struct Reference {bool enabled=true;double scale=1;};
struct Entry {bool enabled=false;double scale=1;std::string referenceHash;std::map<std::string,Reference> acceptedReferences;};
struct Table {std::map<std::string,Entry> meshes;std::string version;};
inline Table Parse(std::string_view input){
 if(input.size()>4*1024*1024)throw std::runtime_error("scale JSON exceeds 4 MiB");
 std::vector<std::set<std::string>> keys;
 const auto j=nlohmann::json::parse(input,[&](int depth,nlohmann::json::parse_event_t event,nlohmann::json& value){
  if(depth>64)throw std::runtime_error("scale JSON too deeply nested");
  if(event==nlohmann::json::parse_event_t::object_start)keys.emplace_back();
  if(event==nlohmann::json::parse_event_t::key && !keys.back().insert(value.get<std::string>()).second)throw std::runtime_error("duplicate JSON key");
  if(event==nlohmann::json::parse_event_t::object_end)keys.pop_back();return true;
 });
 if(!j.is_object() || j.at("schema_version")!=1 || !j.at("meshes").is_object())throw std::runtime_error("unsupported scale table schema");
 const auto& application=j.at("application");
 if(application.at("space")!="mesh_local" || application.at("pivot")!=nlohmann::json::array({0,0,0}) || application.at("strict_reference_sha256")!=true)throw std::runtime_error("unsupported scale application policy");
 Table table;table.version=j.value("table_version",std::string{});
 if(j.at("meshes").size()>8192)throw std::runtime_error("too many mesh entries");
 for(const auto& [name,e]:j.at("meshes").items()){
  if(ResourceKey(name)!=name || !name.ends_with(".msh") || !std::all_of(name.begin(),name.end(),[](char c){return (c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='.';}))throw std::runtime_error("noncanonical mesh key");
  Entry entry;entry.enabled=e.at("enabled").get<bool>();
  if(entry.enabled){entry.scale=e.at("uniform_scale").get<double>();entry.referenceHash=e.at("reference_sha256").get<std::string>();if(!ValidScale(entry.scale) || !Hash(entry.referenceHash))throw std::runtime_error("invalid scale/hash");}
  if(e.contains("accepted_references")){
   const auto& refs=e.at("accepted_references");if(!refs.is_array() || refs.size()>64)throw std::runtime_error("invalid accepted references");
   for(const auto& ref:refs){const auto hash=ref.at("reference_sha256").get<std::string>();Reference variant;variant.enabled=ref.at("enabled").get<bool>();variant.scale=ref.at("uniform_scale").get<double>();
    if(!Hash(hash) || hash==entry.referenceHash || !ValidScale(variant.scale) || !entry.acceptedReferences.emplace(hash,variant).second)throw std::runtime_error("invalid or duplicate reference variant");}
  }
  table.meshes.emplace(name,std::move(entry));
 }return table;
}
struct Config {
 bool tableEnabled=false,creatureEnabled=false,diagnostics=false;
 bool excludeDarkspawnFromCreatureScale=true;
 double unknownWeaponScale=1;
 std::map<std::string,double> creature;
 bool ProbeEnabled()const{return tableEnabled || creatureEnabled || diagnostics;}
};
struct Decision {double table=1,creature=1,final=1;std::string tableReason="disabled",creatureReason="disabled";bool darkspawn=false;};
inline Decision Resolve(const Config& config,const Table& table,std::string_view resource,std::string_view referenceHash,std::string_view creature){
 Decision d;d.darkspawn=IsDarkspawn(resource);
 if(config.tableEnabled){
  const auto it=table.meshes.find(ResourceKey(std::string(resource)));
  if(it==table.meshes.end())d.tableReason="not_listed";
  else if(!it->second.enabled)d.tableReason="entry_disabled";
  else if(Hash(referenceHash) && referenceHash==it->second.referenceHash){d.table=it->second.scale;d.tableReason="matched";}
  else{const auto ref=it->second.acceptedReferences.find(std::string(referenceHash));
   if(ref==it->second.acceptedReferences.end())d.tableReason="reference_mismatch";
   else if(!ref->second.enabled)d.tableReason="reference_disabled";
   else{d.table=ref->second.scale;d.tableReason="matched_custom";}}
 }
 // Explicitly disabled entries stay native. Unknown factor replaces only
 // missing entries or unrecognized content, and only with a resolved MSH.
 if(config.tableEnabled && !resource.empty() && (d.tableReason=="not_listed" || d.tableReason=="reference_mismatch") && ValidScale(config.unknownWeaponScale))d.table=config.unknownWeaponScale;
 if(config.creatureEnabled){
  if(config.excludeDarkspawnFromCreatureScale && d.darkspawn)d.creatureReason="darkspawn_exempt";
  else{const auto it=config.creature.find(std::string(creature));d.creatureReason="unknown_or_invalid";if(it!=config.creature.end() && ValidScale(it->second)){d.creature=it->second;d.creatureReason="configured";}}
 }
 d.final=d.table*d.creature;return d;
}
}
