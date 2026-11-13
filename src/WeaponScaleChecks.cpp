// SPDX-License-Identifier: GPL-3.0-or-later
#include "WeaponScalePolicy.h"
#include <fstream>
#include <iostream>
int main(int argc,char** argv){try{
 unsigned checks=0;const auto check=[&](bool value){++checks;if(!value)throw std::runtime_error("weapon policy check "+std::to_string(checks));};
 check(argc==2);std::ifstream file(argv[1]);const std::string text((std::istreambuf_iterator<char>(file)),{});const auto table=weapon_scale::Parse(text);check(table.meshes.size()==99);
 const auto& [name,entry]=*table.meshes.begin();weapon_scale::Config c;c.creature["HF"]=0.8;
 auto d=weapon_scale::Resolve(c,table,name,entry.referenceHash,"HF");check(d.final==1);
 c.tableEnabled=true;d=weapon_scale::Resolve(c,table,name,entry.referenceHash,"HF");check(d.final==entry.scale);
 c.creatureEnabled=true;d=weapon_scale::Resolve(c,table,name,entry.referenceHash,"HF");check(std::abs(d.final-entry.scale*0.8)<1e-12);
 c.tableEnabled=false;d=weapon_scale::Resolve(c,table,"custom_weapon.msh","","HF");check(d.final==0.8 && d.table==1);
 c.tableEnabled=true;d=weapon_scale::Resolve(c,table,"custom_weapon.msh","","HF");check(d.final==0.8 && d.tableReason=="not_listed");
 d=weapon_scale::Resolve(c,table,name,std::string(64,'0'),"HF");check(d.final==0.8 && d.tableReason=="reference_mismatch");
 auto disabled=table;disabled.meshes.at(name).enabled=false;d=weapon_scale::Resolve(c,disabled,name,entry.referenceHash,"HF");check(d.final==0.8 && d.tableReason=="entry_disabled");
 d=weapon_scale::Resolve(c,table,name,entry.referenceHash,"UNKNOWN");check(d.creature==1 && d.final==entry.scale);
 check(weapon_scale::ResourceKey("C:\\weapons\\W_STF_ST01A_0.MSH")=="w_stf_st01a_0.msh");
 for(const auto& [resource,e]:table.meshes){check(e.enabled && weapon_scale::Hash(e.referenceHash) && weapon_scale::ValidScale(e.scale));check(weapon_scale::Resolve(c,table,resource,e.referenceHash,"HF").table==e.scale);}
 const auto rejects=[&](const std::string& s){try{weapon_scale::Parse(s);return false;}catch(...){return true;}};
 check(rejects("{\"schema_version\":1,\"schema_version\":1}"));
 auto j=nlohmann::json::parse(text);j["meshes"][name]["uniform_scale"]=-1;check(rejects(j.dump()));
 j=nlohmann::json::parse(text);j["application"]["strict_reference_sha256"]=false;check(rejects(j.dump()));
 j=nlohmann::json::parse(text);j["meshes"][name]["reference_sha256"]="bad";check(rejects(j.dump()));
 j=nlohmann::json::parse(text);j["application"]["pivot"]={1,0,0};check(rejects(j.dump()));
 check(rejects("["));check(rejects(std::string(4*1024*1024+1,' ')));
 c.creature["HF"]=std::numeric_limits<double>::infinity();check(weapon_scale::Resolve(c,table,"missing.msh","","HF").final==1);
 // Known original-sized custom meshes scale once; already scaled meshes remain at 1.
 for(const auto& resource:{"w_stf_st01.msh","w_stf_st01a_0.msh"}){
  const auto& custom=table.meshes.at(resource);check(custom.acceptedReferences.size()==4);
  for(const auto& [hash,ref]:custom.acceptedReferences){c.creature["HF"]=0.8;c.tableEnabled=true;c.creatureEnabled=true;
   auto result=weapon_scale::Resolve(c,table,resource,hash,"HF");check(result.tableReason=="matched_custom" && result.table==ref.scale);check(std::abs(result.final-ref.scale*0.8)<1e-12);
   auto off=table;off.meshes.at(resource).acceptedReferences.at(hash).enabled=false;result=weapon_scale::Resolve(c,off,resource,hash,"HF");check(result.table==1 && result.final==0.8 && result.tableReason=="reference_disabled");
  }
 }
 j=nlohmann::json::parse(text);auto& refs=j["meshes"]["w_stf_st01.msh"]["accepted_references"];refs.push_back(refs[0]);check(rejects(j.dump()));
 j=nlohmann::json::parse(text);j["meshes"]["w_stf_st01.msh"]["accepted_references"][0]["uniform_scale"]=0;check(rejects(j.dump()));
 j=nlohmann::json::parse(text);j["meshes"]["w_stf_st01.msh"]["accepted_references"][0]["reference_sha256"]="bad";check(rejects(j.dump()));
 auto legacy=nlohmann::json::parse(text);for(auto& e:legacy["meshes"])e.erase("accepted_references");check(weapon_scale::Parse(legacy.dump()).meshes.size()==99);
 // Darkspawn exemption follows the weapon mesh, independently of table
 // membership/hash, table enabled state, and the race of its wielder.
 weapon_scale::Config ds;ds.tableEnabled=true;ds.creatureEnabled=true;ds.creature["HF"]=.8;ds.creature["HM"]=1.2;
 check(ds.excludeDarkspawnFromCreatureScale);
 for(const auto mesh:weapon_scale::DarkspawnMeshes){
  check(weapon_scale::IsDarkspawn(mesh));const auto it=table.meshes.find(std::string(mesh));const auto hash=it==table.meshes.end()?std::string{}:it->second.referenceHash;
  const auto factor=it==table.meshes.end()?1.:it->second.scale;
  auto result=weapon_scale::Resolve(ds,table,mesh,hash,"HF");check(result.darkspawn && result.creatureReason=="darkspawn_exempt" && result.creature==1 && result.table==factor && result.final==factor);
  result=weapon_scale::Resolve(ds,table,mesh,hash,"HM");check(result.final==factor && result.creature==1);
  ds.tableEnabled=false;result=weapon_scale::Resolve(ds,{},mesh,"","HF");check(result.final==1 && result.creatureReason=="darkspawn_exempt");ds.tableEnabled=true;
  result=weapon_scale::Resolve(ds,table,mesh,std::string(64,'0'),"HF");check(result.final==1 && result.creature==1);
  ds.excludeDarkspawnFromCreatureScale=false;result=weapon_scale::Resolve(ds,table,mesh,hash,"HF");check(std::abs(result.final-factor*.8)<1e-12 && result.creatureReason=="configured");ds.excludeDarkspawnFromCreatureScale=true;
 }
 check(weapon_scale::IsDarkspawn("C:\\override\\W_STF_DSSTA_0.MSH"));
 check(!weapon_scale::IsDarkspawn("w_stf_st01a_0.msh"));check(!weapon_scale::IsDarkspawn("custom_darkspawn_sword.msh"));check(!weapon_scale::IsDarkspawn("w_lsw_dssa_0.msh.backup"));
 ds.tableEnabled=false;check(weapon_scale::Resolve(ds,{},"w_stf_st01a_0.msh","","HF").final==.8);
 // Optional unknown factor is the table-side fallback, not an extra
 // multiplier on known entries. Defaults and explicit exclusions are retained.
 weapon_scale::Config fallback;fallback.tableEnabled=true;fallback.creatureEnabled=true;fallback.creature["HF"]=.8;fallback.unknownWeaponScale=.9;
 auto unknown=weapon_scale::Resolve(fallback,table,"custom_new.msh","","HF");check(unknown.table==.9 && std::abs(unknown.final-.72)<1e-12);
 unknown=weapon_scale::Resolve(fallback,table,name,std::string(64,'0'),"HF");check(unknown.table==.9 && unknown.tableReason=="reference_mismatch");
 check(weapon_scale::Resolve(fallback,table,name,entry.referenceHash,"HF").table==entry.scale);
 check(weapon_scale::Resolve(fallback,disabled,name,entry.referenceHash,"HF").table==1);
 check(weapon_scale::Resolve(fallback,table,"","","HF").table==1);
 fallback.tableEnabled=false;check(weapon_scale::Resolve(fallback,table,"custom_new.msh","","HF").final==.8);fallback.tableEnabled=true;
 fallback.unknownWeaponScale=std::numeric_limits<double>::quiet_NaN();check(weapon_scale::Resolve(fallback,table,"custom_new.msh","","HF").table==1);
 fallback.unknownWeaponScale=.9;unknown=weapon_scale::Resolve(fallback,{},"w_stf_dssta_0.msh","","HF");check(unknown.table==.9 && unknown.creature==1 && unknown.final==.9);
 const auto& apc=table.meshes.at("w_stf_apca_0.msh");check(apc.scale==.85 && weapon_scale::Resolve(fallback,table,"w_stf_apca_0.msh",apc.referenceHash,"HF").table==.85);
 std::cout<<"PASS: "<<checks<<" weapon scale policy checks; 99 entries, independent switches, hash/disabled/unknown and malformed JSON\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
