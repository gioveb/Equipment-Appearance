#include "CinematicCamera.h"
#include <iostream>
#include <stdexcept>
#include <limits>
unsigned checks=0;
void Check(bool x){++checks;if(!x)throw std::runtime_error("cinematic matrix/shot check failed");}
int main(){try{
 heels::Matrix view{1,0,0,0,0,0,1,-1.5f,0,-1,0,0,0,0,0,1},world{};
 Check(heels::CameraWorld(view,world));
 std::array<heels::CameraSubject,2> subjects{{{100,{0,4,1.5f,1},.08f,true},{82,{2,1,1.5f,1},.05f,true}}};
 auto c=heels::ChooseShot(world,subjects,0xffffffff);Check(c.valid && c.id==100 && c.height==.08f);
 subjects[0].height=0;c=heels::ChooseShot(world,subjects,0xffffffff);Check(c.valid && c.id==100 && c.height==0);
 subjects[0].height=.08f;subjects[1].nativeHead={0,6,1.5f,1};
 Check(!heels::ChooseShot(world,subjects,0xffffffff).valid);
 c=heels::ChooseShot(world,subjects,82);Check(c.valid && c.id==82 && c.held);
 subjects[0].valid=false;subjects[1].valid=false;Check(!heels::ChooseShot(world,subjects,82).valid);
 const auto saved=view;
 for(float h:{0.f,.05f,.08f,.2f})for(unsigned i=0;i<1000;++i){auto raised=heels::RaisedView(view,h);heels::Matrix w{};Check(heels::CameraWorld(raised,w));Check(std::abs(w[11]-world[11]-h)<.000001f);Check(w[3]==world[3] && w[7]==world[7]);Check(view==saved);}
 auto bad=view;bad[0]=2;Check(!heels::CameraWorld(bad,world));bad=view;bad[3]=std::numeric_limits<float>::infinity();Check(!heels::CameraWorld(bad,world));
 std::cout<<"PASS: "<<checks<<" cinematic checks; world-Z translation, no accumulation, inferred target/ambiguity/zero-height handling and invalid matrices\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
