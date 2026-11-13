#pragma once
#include "Offset.h"
#include <cstdint>
namespace heels {
struct DialogueState {
 std::uintptr_t mode=0;std::uint64_t expires=0;
 std::uint32_t focus=0xffffffff,hero=0xffffffff;bool scripted=false,ending=false;
 bool Allows(std::uint32_t actor,std::uint64_t now)const{
  return mode && scripted && now<expires && actor!=0xffffffff && (actor==focus || actor==hero);
 }
};
inline bool ShiftPose(const Matrix& parent,const Matrix& model,const Matrix& inverse,const std::array<float,4>& logical,float metres,bool allowStaging,Frame& out,bool& staged){
 staged=false;
 if(ShiftFrame(parent,model,inverse,logical,metres,out))return true;
 if(!allowStaging)return false;
 float distance2=0;
 for(unsigned i=0;i<3;++i){if(!std::isfinite(logical[i]) || !std::isfinite(model[4*i+3]))return false;const float delta=model[4*i+3]-logical[i];distance2+=delta*delta;}
 // A nearby authored placement of a verified dialogue participant, not an origin-space preview.
 if(!std::isfinite(distance2) || distance2>9.f)return false;
 const std::array<float,4> nativePosition{model[3],model[7],model[11],logical[3]};
 if(!ShiftFrame(parent,model,inverse,nativePosition,metres,out))return false;
 staged=true;return true;
}
}
