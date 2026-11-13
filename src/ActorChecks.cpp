#include "Actors.h"
#include <iostream>
#include <stdexcept>
unsigned checks=0;void Check(bool b){++checks;if(!b)throw std::runtime_error("actor registry check failed");}
int main(){try{
 heels::Actors r(3);heels::Actor a{1,10,100,1000},b{2,10,200,2000};
 Check(r.Upsert(a,1));Check(r.Upsert(b,2));Check(r.Find(1000)==a);Check(r.Find(2000)==b);
 const std::array<float,4> apos{10,20,30,1},bpos{40,50,60,1};
 r.Rendered(a,0.05f,10,apos);r.Rendered(b,0.08f,10,bpos);
 Check(r.NativePosition(a,11)==apos);Check(r.NativePosition(b,11)==bpos);Check(!r.NativePosition(a,1011));
 Check(r.RenderedHeight(a,10)==0.05f);Check(r.RenderedHeight(b,10)==0.08f);
 Check(r.RaisedCandidates().size()==2);
 Check(r.RenderedHeight(a,9)==0);Check(r.RenderedHeight(a,1011)==0);
 r.Block(a);Check(!r.Find(1000));Check(r.Find(2000)==b);
 Check(r.RenderedHeight(a,11)==0);Check(r.RenderedHeight(b,11)==0.08f);
 Check(r.RaisedCandidates().size()==1 && r.RaisedCandidates()[0]==b);
 Check(!r.Upsert(a,3));Check(!r.Find(1000));
 auto newer=a;newer.model=1100;Check(r.Upsert(newer,4));Check(!r.Find(1000));Check(r.Find(1100)==newer);
 Check(r.RenderedHeight(newer,12)==0);r.Rendered(a,0.2f,12);Check(r.RenderedHeight(newer,12)==0);
 r.Erase(a);Check(r.Find(1100)==newer);r.Erase(newer);Check(!r.Find(1100));Check(r.Find(2000)==b);
 auto reused=b;reused.creature=201;reused.model=2200;reused.area=11;Check(r.Upsert(reused,5));Check(!r.Find(2000));r.Erase(b);Check(r.Find(2200)==reused);
 heels::Actor collision{3,11,300,2200};Check(r.Upsert(collision,6));Check(r.Find(2200)==collision);Check(r.Size()==1);
 for(unsigned i=10;i<1000;++i){heels::Actor v{i,20,10000+i,100000+i};Check(r.Upsert(v,i));Check(r.Size()<=3);Check(r.Find(v.model)==v);}
 Check(!r.Find(100010));const auto snapshot=r.Snapshot();Check(snapshot.size()==3);
 for(const auto& v:snapshot)r.Erase(v);Check(r.Size()==0);
 Check(!r.Upsert({},1001));
 std::cout<<"PASS: "<<checks<<" actor registry checks; independent models, blocking isolation, replacement, reused IDs, stale removal and bounded capacity\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
