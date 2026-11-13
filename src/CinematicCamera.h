#pragma once
#include "Offset.h"
#include <cstdint>
namespace heels {
inline bool NearMatrix(const Matrix& a,const Matrix& b){for(unsigned i=0;i<16;++i)if(!std::isfinite(a[i]) || !std::isfinite(b[i]) || std::abs(a[i]-b[i])>0.0002f)return false;return true;}
inline bool CameraWorld(const Matrix& view,Matrix& out){
 for(float v:view)if(!std::isfinite(v))return false;
 if(std::abs(view[12])>0.001f || std::abs(view[13])>0.001f || std::abs(view[14])>0.001f || std::abs(view[15]-1)>0.001f)return false;
 for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c){float dot=0;for(unsigned k=0;k<3;++k)dot+=view[r*4+k]*view[c*4+k];if(std::abs(dot-(r==c?1.f:0.f))>0.002f)return false;}
 out={};out[15]=1;
 for(unsigned r=0;r<3;++r){for(unsigned c=0;c<3;++c)out[r*4+c]=view[c*4+r];out[r*4+3]=-(out[r*4]*view[3]+out[r*4+1]*view[7]+out[r*4+2]*view[11]);}
 return true;
}
inline Matrix RaisedView(const Matrix& native,float metres){
 Matrix out=native;for(unsigned r=0;r<4;++r)out[4*r+3]-=metres*native[4*r+2];return out;
}
struct CameraSubject{std::uint32_t id=0xffffffff;std::array<float,4> nativeHead{};float height=0;bool valid=false;};
struct ShotChoice{std::uint32_t id=0xffffffff;float height=0;bool valid=false,held=false;};
// Geometric inference, deliberately restricted to two known dialogue participants.
inline ShotChoice ChooseShot(const Matrix& world,const std::array<CameraSubject,2>& subjects,std::uint32_t previous){
 struct Ranked{unsigned index;float score;};std::array<Ranked,2> ranked{};unsigned count=0;
 const float fx=-world[2],fy=-world[6],fz=-world[10],fl=std::sqrt(fx*fx+fy*fy+fz*fz);
 if(!std::isfinite(fl) || fl<0.001f)return {};
 for(unsigned i=0;i<2;++i){const auto& s=subjects[i];if(!s.valid || s.id==0xffffffff || !std::isfinite(s.height) || s.height<0 || s.height>0.2f)continue;
  const float dx=s.nativeHead[0]-world[3],dy=s.nativeHead[1]-world[7],dz=s.nativeHead[2]-world[11];const float length=std::sqrt(dx*dx+dy*dy+dz*dz);
  if(!std::isfinite(length) || length<0.05f || length>25.f)continue;
  const float cosine=(dx*fx+dy*fy+dz*fz)/(length*fl);if(!std::isfinite(cosine) || cosine<0.85f)continue;
  ranked[count++]={i,1.f-cosine*cosine};
 }
 if(!count)return {};
 if(count==2 && ranked[1].score<ranked[0].score)std::swap(ranked[0],ranked[1]);
 unsigned index=ranked[0].index;bool held=false;
 if(count==2 && ranked[1].score-ranked[0].score<0.02f){
  bool found=false;for(const auto& r:ranked)if(subjects[r.index].id==previous && r.score<0.1f){index=r.index;found=true;held=true;break;}
  if(!found)return {};
 }
 const auto& selected=subjects[index];return {selected.id,selected.height,true,held};
}
}
