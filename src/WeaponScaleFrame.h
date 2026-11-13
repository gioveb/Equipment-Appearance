// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "Offset.h"

namespace weapon_scale {
// Column-vector affine convention, translation in 3/7/11.
// P' = P S; model M and inverse(M) remain native.
// The third native argument belongs to the enclosing traversal frame; it
// must not be assumed to invert the weapon attachment matrix M. Both
// native reference arguments are forwarded untouched.
// The origin and world translation stay fixed. Never modify native inputs.
inline bool ScaleFrame(const heels::Matrix& parent,const heels::Matrix& model,
                       const heels::Matrix& inverse,double scale,heels::Frame& out){
 if(!std::isfinite(scale) || scale<0.0025 || scale>100)return false;
 for(unsigned i=0;i<16;++i){
  if(!std::isfinite(parent[i]) || !std::isfinite(model[i]) || !std::isfinite(inverse[i]))return false;

 }
 for(const auto* a:{&parent,&model,&inverse})
  if(std::abs((*a)[12])>0.0001f || std::abs((*a)[13])>0.0001f || std::abs((*a)[14])>0.0001f || std::abs((*a)[15]-1)>0.0001f)return false;
 heels::Frame candidate;candidate.parent=parent;candidate.model=model;candidate.inverse=inverse;
 // Scale local axes, never the attachment translation. The reference model
 // can differ from the incoming parent during animation; native model-space
 // conversion still uses the original, consistent model/inverse pair.
 for(unsigned row=0;row<3;++row)
  for(unsigned col=0;col<3;++col)candidate.parent[row*4+col]*=static_cast<float>(scale);
 for(const auto* a:{&candidate.parent,&candidate.model,&candidate.inverse})for(float value:*a)if(!std::isfinite(value))return false;
 out=candidate;return true;
}
}
