// SPDX-License-Identifier: GPL-3.0-or-later
#include "WeaponScaleFrame.h"
#include <iostream>
#include <stdexcept>
#include <limits>
unsigned checks=0;
void Check(bool b){++checks;if(!b)throw std::runtime_error("weapon frame check "+std::to_string(checks));}
bool Near(float a,float b){return std::abs(a-b)<0.001f;}
int main(){try{
 const heels::Matrix identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
 for(float angle:{0.f,.3f,1.7f,3.1f})for(float tilt:{0.f,.7f,2.f})for(float base:{.7f,1.f,1.5f}){
  const float c=std::cos(angle),s=std::sin(angle),ct=std::cos(tilt),st=std::sin(tilt);
  const heels::Matrix rz{c,-s,0,0,s,c,0,0,0,0,1,0,0,0,0,1},rx{1,0,0,0,0,ct,-st,0,0,st,ct,0,0,0,0,1};
  auto m=heels::Multiply(rz,rx);for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)m[i*4+j]*=base;
  m[3]=217.3f;m[7]=335.1f;m[11]=14.8f;
  auto inv=identity;for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)inv[i*4+j]=m[j*4+i]/(base*base);
  for(unsigned i=0;i<3;++i)inv[i*4+3]=-(inv[i*4]*m[3]+inv[i*4+1]*m[7]+inv[i*4+2]*m[11]);
  const auto original=m,originalInverse=inv;
  for(double factor:{.05,.5,.85,1.,1.2,10.}){
   heels::Frame scaled;Check(weapon_scale::ScaleFrame(m,m,inv,factor,scaled));
   for(unsigned row=0;row<3;++row){Check(scaled.parent[row*4+3]==m[row*4+3]);Check(scaled.model==m && scaled.inverse==inv);}
   for(const auto& product:{heels::Multiply(scaled.model,scaled.inverse),heels::Multiply(scaled.inverse,scaled.model)})
    for(unsigned i=0;i<16;++i)Check(Near(product[i],identity[i]));
   // Several local vertices, including the local origin: world(point)-origin scales.
   for(const auto& v:{std::array<float,3>{0,0,0},std::array<float,3>{.2f,-.1f,1.05f},std::array<float,3>{-.1f,.2f,-1.05f}})
    for(unsigned row=0;row<3;++row){float old=m[row*4+3],value=scaled.parent[row*4+3];for(unsigned k=0;k<3;++k){old+=m[row*4+k]*v[k];value+=scaled.parent[row*4+k]*v[k];}Check(Near(value,m[row*4+3]+float(factor)*(old-m[row*4+3])));}
   // Recursive child transform: inherited scaled parent, no second scale.
   auto child=identity;child[3]=.01f;child[7]=.003f;child[11]=.006f;
   const auto world=heels::Multiply(scaled.parent,child);const auto oldWorld=heels::Multiply(m,child);
   for(unsigned row=0;row<3;++row)Check(Near(world[row*4+3],m[row*4+3]+float(factor)*(oldWorld[row*4+3]-m[row*4+3])));
   for(unsigned repeat=0;repeat<100;++repeat){heels::Frame next;Check(weapon_scale::ScaleFrame(m,m,inv,factor,next));Check(next.parent==scaled.parent && next.model==m && next.inverse==inv);}
   Check(m==original && inv==originalInverse);
  }
 }
 heels::Frame out;Check(!weapon_scale::ScaleFrame(identity,identity,identity,0,out));Check(!weapon_scale::ScaleFrame(identity,identity,identity,-1,out));Check(!weapon_scale::ScaleFrame(identity,identity,identity,std::numeric_limits<double>::quiet_NaN(),out));
 auto bad=identity;bad[3]=1;Check(weapon_scale::ScaleFrame(bad,identity,identity,.85,out));Check(out.parent[3]==1 && out.model==identity);Check(weapon_scale::ScaleFrame(identity,identity,bad,.85,out));Check(out.inverse==bad && out.model==identity);
 bad=identity;bad[0]=std::numeric_limits<float>::infinity();Check(!weapon_scale::ScaleFrame(bad,bad,identity,.85,out));
 // Independent instance factors applied to the same incoming frame.
 heels::Frame a,b;Check(weapon_scale::ScaleFrame(identity,identity,identity,.85,a));Check(weapon_scale::ScaleFrame(identity,identity,identity,.68,b));Check(Near(a.parent[0],.85f) && Near(b.parent[0],.68f));
 // Reproduce the engine caching each forwarded model reference, while the
 // next animation update supplies fresh native parent/inverse matrices.
 // Test 1 scaled that reference; the following frame could then be rejected.
 auto cachedModel=identity;heels::Frame steady;
 for(unsigned tick=0;tick<1000;++tick){auto animatedParent=identity;animatedParent[3]=float(tick)*.01f;animatedParent[11]=.08f;
  Check(weapon_scale::ScaleFrame(animatedParent,cachedModel,identity,.85,steady));
  Check(Near(steady.parent[0],.85f) && steady.parent[3]==animatedParent[3] && steady.parent[11]==.08f);
  cachedModel=steady.model;Check(cachedModel==identity && steady.inverse==identity);
 }
 // World attachments may receive a traversal reference that does not
 // invert their attachment M. It must pass through unchanged each frame.
 auto traversal=identity;traversal[3]=-216.7f;traversal[7]=-334.8f;traversal[11]=-14.1f;
 auto weapon=identity;weapon[3]=217.3f;weapon[7]=335.1f;weapon[11]=14.8f;
 for(unsigned tick=0;tick<1000;++tick){auto parent=weapon;parent[3]+=float(tick)*.001f;
  Check(weapon_scale::ScaleFrame(parent,weapon,traversal,.85,steady));
  Check(steady.model==weapon && steady.inverse==traversal);
  Check(steady.parent[3]==parent[3] && Near(steady.parent[0],.85f));
 }
 std::cout<<"PASS: "<<checks<<" weapon local-frame checks; rotated/tilted/scaled attachments, fixed origin, consistent inverse, child transforms, no accumulation, unchanged native cached reference, animation frames and independent instances\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
