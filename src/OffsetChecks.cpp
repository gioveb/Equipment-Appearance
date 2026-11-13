#include "Offset.h"
#include <iostream>
#include <limits>
#include <stdexcept>
unsigned checks=0;void Check(bool b){++checks;if(!b)throw std::runtime_error("offset check failed");}
int main(){try{
 for(unsigned n=0;n<360;++n){
  const float a=n*0.0174532925f;heels::Matrix m{std::cos(a),-std::sin(a),0,100,std::sin(a),std::cos(a),0,200,0,0,1,12,0,0,0,1};
  const auto original=m;const std::array<float,4> logical{100,200,12,1};heels::Matrix out{};
  Check(heels::Offset(m,m,logical,0.1f,out));Check(m==original);
  for(unsigned k=0;k<16;++k)Check(std::abs(out[k]-(original[k]+(k==11?0.1f:0)))<0.00001f);
  heels::Matrix again{};Check(heels::Offset(m,m,logical,0.1f,again));Check(again==out);
 }
 heels::Matrix m{1,0,0,100,0,1,0,200,0,0,1,12,0,0,0,1},out{};const std::array<float,4> logical{100,200,12,1};
 Check(!heels::Offset(m,m,logical,-0.1f,out));Check(!heels::Offset(m,m,logical,0.21f,out));Check(!heels::Offset(m,m,logical,std::numeric_limits<float>::quiet_NaN(),out));
 auto bad=m;bad[11]=0;Check(!heels::Offset(bad,bad,logical,0.1f,out));
 bad=m;bad[8]=0.1f;Check(!heels::Offset(bad,bad,logical,0.1f,out));
 bad=m;bad[15]=0;Check(!heels::Offset(bad,bad,logical,0.1f,out));
 bad=m;bad[3]+=0.1f;Check(!heels::Offset(m,bad,logical,0.1f,out));
 for(unsigned k=0;k<16;++k){bad=m;bad[k]=std::numeric_limits<float>::infinity();Check(!heels::Offset(bad,bad,logical,0.1f,out));}
 for(unsigned n=0;n<360;++n)for(float scale:{0.8f,1.f,1.2f}){
  const float a=n*0.0174532925f,c=std::cos(a),s=std::sin(a);
  heels::Matrix model{scale*c,-scale*s,0,100,scale*s,scale*c,0,200,0,0,scale,12,0,0,0,1};
  heels::Matrix inv{c/scale,s/scale,0,(-100*c-200*s)/scale,-s/scale,c/scale,0,(100*s-200*c)/scale,0,0,1/scale,-12/scale,0,0,0,1};
  const auto savedModel=model,savedInverse=inv;heels::Frame frame;
  Check(heels::ShiftFrame(model,model,inv,logical,0.05f,frame));
  Check(model==savedModel && inv==savedInverse);
  auto bone=heels::Matrix{1,0,0,0.2f,0,1,0,0.1f,0,0,1,1.3f,0,0,0,1};
  const auto world0=heels::Multiply(model,bone);
  const auto world1=heels::Multiply(frame.parent,bone);
  const auto local0=heels::Multiply(inv,world0);
  const auto local1=heels::Multiply(frame.inverse,world1);
  const auto rendered=heels::Multiply(frame.model,local1);
  for(unsigned k=0;k<16;++k){Check(std::abs(local0[k]-local1[k])<0.0002f);Check(std::abs(rendered[k]-(world0[k]+(k==11?0.05f:0)))<0.0002f);}
  heels::Frame again;Check(heels::ShiftFrame(model,model,inv,logical,0.05f,again));Check(again.parent==frame.parent && again.model==frame.model && again.inverse==frame.inverse);
  inv[11]+=0.2f;Check(!heels::ShiftFrame(model,model,inv,logical,0.05f,again));
 }
 std::cout<<"PASS: "<<checks<<" offset/frame checks; skin/model-space invariance, single world-space displacement, rotations/scales, unchanged input and inverse rejection\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
