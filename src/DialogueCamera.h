#pragma once
#include <array>
#include <cmath>
#include <cstdint>
namespace heels {
using CameraVector=std::array<float,4>;
struct CameraContext {std::uintptr_t camera=0;std::uint32_t actor=0xffffffff;float height=0;};
struct CameraScope {
 CameraContext& current;CameraContext previous;
 CameraScope(CameraContext& ref,CameraContext value):current(ref),previous(ref){current=value;}
 ~CameraScope(){current=previous;}
};
inline bool Compensate(const CameraContext& scope,std::uintptr_t camera,const CameraVector& input,CameraVector& output){
 if(!scope.camera || scope.camera!=camera || scope.actor==0xffffffff || !std::isfinite(scope.height) || scope.height<=0 || scope.height>0.2f)return false;
 for(unsigned i=0;i<3;++i)if(!std::isfinite(input[i]))return false;
 output=input;output[2]+=scope.height;return std::isfinite(output[2]);
}
}
