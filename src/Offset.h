#pragma once
#include <array>
#include <cmath>
namespace heels {
using Matrix=std::array<float,16>;
// DAO's matrix translation is at 3,7,11. Never edit the engine's input matrices.
inline bool Offset(const Matrix& parent,const Matrix& model,const std::array<float,4>& logical,float metres,Matrix& out){
 if(!std::isfinite(metres) || metres<0 || metres>0.2f)return false;
 for(unsigned i=0;i<16;++i)if(!std::isfinite(parent[i]) || !std::isfinite(model[i]) || std::abs(parent[i]-model[i])>0.0001f)return false;
 for(unsigned i=0;i<3;++i)if(!std::isfinite(logical[i]) || std::abs(model[4*i+3]-logical[i])>0.002f)return false;
 if(std::abs(parent[12])>0.0001f || std::abs(parent[13])>0.0001f || std::abs(parent[14])>0.0001f || std::abs(parent[15]-1)>0.0001f)return false;
 // Prototype excludes tilted/downed roots; this is not a complete animation classifier.
 if(std::abs(parent[8])>0.001f || std::abs(parent[9])>0.001f || parent[10]<0.5f || parent[10]>2.f)return false;
 out=parent;out[11]+=metres;return std::isfinite(out[11]);
}
inline Matrix Multiply(const Matrix& a,const Matrix& b){
 Matrix out{};for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)for(unsigned k=0;k<4;++k)out[r*4+c]+=a[r*4+k]*b[k*4+c];return out;
}
struct alignas(16) Frame {alignas(16) Matrix parent{},model{},inverse{};};
inline bool ShiftFrame(const Matrix& parent,const Matrix& model,const Matrix& inverse,const std::array<float,4>& logical,float metres,Frame& out){
 if(!Offset(parent,model,logical,metres,out.parent))return false;
 for(float x:inverse)if(!std::isfinite(x))return false;
 // Reject a mismatched inverse rather than attempting to repair an unknown frame.
 const auto identity=Multiply(model,inverse);
 for(unsigned i=0;i<16;++i)if(!std::isfinite(identity[i]) || std::abs(identity[i]-(i%5==0?1.f:0.f))>0.001f)return false;
 out.model=model;out.model[11]+=metres;
 // M' = T(h) M; inverse(M') = inverse(M) T(-h).
 out.inverse=inverse;
 for(unsigned r=0;r<4;++r)out.inverse[4*r+3]-=metres*inverse[4*r+2];
 return true;
}
}
