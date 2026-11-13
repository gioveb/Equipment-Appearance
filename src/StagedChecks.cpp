#include "StagedPose.h"
#include <iostream>
#include <stdexcept>
unsigned checks=0;void Check(bool b){++checks;if(!b)throw std::runtime_error("staged pose check failed");}
int main(){try{
 heels::DialogueState state{0x1000,2000,0x1ddd,0x82,true};
 Check(state.Allows(0x82,1000));Check(state.Allows(0x1ddd,1000));Check(!state.Allows(0x83,1000));Check(!state.Allows(0xffffffff,1000));Check(!state.Allows(0x82,2000));
 state.scripted=false;Check(!state.Allows(0x82,1000));state.scripted=true;
 const std::array<float,4> logical{214.55699f,322.03415f,13.09048f,1};
 for(unsigned n=0;n<360;++n){
  const float angle=n*0.0174532925f,c=std::cos(angle),s=std::sin(angle);
  // Exact type of staged displacement reported by the user's log.
  const float x=214.90936f,y=322.30484f,z=13.11795f;
  heels::Matrix model{c,-s,0,x,s,c,0,y,0,0,1,z,0,0,0,1};
  heels::Matrix inverse{c,s,0,-c*x-s*y,-s,c,0,s*x-c*y,0,0,1,-z,0,0,0,1};
  heels::Frame frame{};bool staged=false;
  Check(!heels::ShiftPose(model,model,inverse,logical,0.05f,false,frame,staged));
  Check(heels::ShiftPose(model,model,inverse,logical,0.05f,true,frame,staged));Check(staged);
  Check(std::abs(frame.model[11]-model[11]-0.05f)<0.00001f);
  const auto identity=heels::Multiply(frame.inverse,frame.parent);
  for(unsigned i=0;i<16;++i)Check(std::abs(identity[i]-(i%5==0?1.f:0.f))<0.001f);
  const auto before=model;heels::Frame again{};Check(heels::ShiftPose(model,model,inverse,logical,0.05f,true,again,staged));Check(again.model==frame.model && model==before);
  auto wrongParent=model;wrongParent[3]+=0.1f;Check(!heels::ShiftPose(wrongParent,model,inverse,logical,0.05f,true,again,staged));
  auto wrongInverse=inverse;wrongInverse[11]+=0.1f;Check(!heels::ShiftPose(model,model,wrongInverse,logical,0.05f,true,again,staged));
  auto far=logical;far[0]-=10;Check(!heels::ShiftPose(model,model,inverse,far,0.05f,true,again,staged));
 }
 // Regression: real 0.0.11 rejection is a valid 2.05m authored dialogue placement.
 const std::array<float,4> logical12{218.84892f,337.12988f,13.09543f,1};
 heels::Matrix real{1,0,0,220.59322f,0,1,0,336.06259f,0,0,1,13.14724f,0,0,0,1};
 auto inv=real;inv[3]=-real[3];inv[7]=-real[7];inv[11]=-real[11];heels::Frame frame{};bool staged=false;
 Check(heels::ShiftPose(real,real,inv,logical12,.05f,true,frame,staged));Check(staged);Check(std::abs(frame.model[11]-13.19724f)<.00001f);
 for(unsigned i=0;i<16;++i)if(i!=11)Check(frame.model[i]==real[i]);
 auto remote=logical12;remote[0]=real[3]-3.1f;remote[1]=real[7];Check(!heels::ShiftPose(real,real,inv,remote,.05f,true,frame,staged));
 std::cout<<"PASS: "<<checks<<" staged pose checks; participant/expiry gates, real displacement fixture, matrix/inverse consistency, no accumulation and invalid/remote frame rejection\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
