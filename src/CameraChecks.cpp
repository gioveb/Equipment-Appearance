// Exercises the actual production setter detours with mock native callees.
#include "Probe.cpp"
#include <iostream>
#include <stdexcept>
namespace {
unsigned checks=0,positionCalls=0,lookCalls=0;
heels::CameraVector positionValue{},lookValue{};const void* received=nullptr;const void* receivedOther=nullptr;
std::uint32_t receivedActor=0,receivedMode=0;float receivedDuration=0;
unsigned viewCalls=0;const void* viewPointer=nullptr;heels::Matrix viewValue{};
void Assert(bool b){++checks;if(!b)throw std::runtime_error("production camera callback check failed");}
void __fastcall PositionMock(void*,int,const void* p,float t,std::uint32_t mode){++positionCalls;received=p;std::memcpy(positionValue.data(),p,16);receivedDuration=t;receivedMode=mode;}
void __fastcall LookMock(void*,int,std::uint32_t actor,const void* p,const void* other){++lookCalls;received=p;std::memcpy(lookValue.data(),p,16);receivedActor=actor;receivedOther=other;}
std::uintptr_t __fastcall ViewMock(void*,int,const void* p){++viewCalls;viewPointer=p;std::memcpy(viewValue.data(),p,64);return 0xabcd1234u;}
std::uintptr_t __fastcall BeginMock(void*,int){Assert(stageState.mode==0);return 0x11223344u;}
std::uintptr_t __fastcall EndMock(void*,int,std::int32_t result){Assert(result==7);return 0x55667788u;}
}
int main(){try{
 spdlog::set_level(spdlog::level::off);cameraPositionOriginal=&PositionMock;cameraLookOriginal=&LookMock;
 auto* camera=reinterpret_cast<void*>(0x1000);auto* otherCamera=reinterpret_cast<void*>(0x2000);
 alignas(16) heels::CameraVector position{10,20,30,1},offset{0.1f,0.2f,1.5f,0},other{3,4,5,0};
 const auto originalPosition=position,originalOffset=offset;
 for(float h:{0.05f,0.08f,0.2f})for(unsigned repeat=0;repeat<100;++repeat){
  heels::CameraScope scope(dialogueCamera,{0x1000,82,h});
  CameraPosition(camera,0,position.data(),1.25f,0x11223344);
  Assert(received!=position.data());Assert(receivedDuration==1.25f && receivedMode==0x11223344);
  Assert(positionValue[0]==10 && positionValue[1]==20 && positionValue[3]==1);Assert(std::abs(positionValue[2]-(30+h))<0.00001f);
  CameraLook(camera,0,82,offset.data(),other.data());
  Assert(received!=offset.data());Assert(receivedActor==82 && receivedOther==other.data());
  Assert(lookValue[0]==offset[0] && lookValue[1]==offset[1] && lookValue[3]==offset[3]);Assert(std::abs(lookValue[2]-(offset[2]+h))<0.00001f);
  Assert(position==originalPosition && offset==originalOffset);
  // Same displacement for camera and world look target preserves the native aim direction.
  Assert(std::abs((100+lookValue[2]-positionValue[2])-(100+offset[2]-position[2]))<0.00002f);
  CameraPosition(otherCamera,0,position.data(),0,7);Assert(received==position.data() && positionValue==position);
  CameraLook(camera,0,99,offset.data(),nullptr);Assert(received==offset.data() && lookValue==offset && receivedOther==nullptr);
 }
 Assert(dialogueCamera.camera==0);
 CameraPosition(camera,0,position.data(),0,0);Assert(received==position.data());
 CameraLook(camera,0,82,offset.data(),nullptr);Assert(received==offset.data());
 for(float h:{0.f,-0.1f,0.21f}){heels::CameraScope scope(dialogueCamera,{0x1000,82,h});CameraPosition(camera,0,position.data(),0,0);Assert(received==position.data());}
 try{heels::CameraScope outer(dialogueCamera,{0x1000,82,0.05f});{heels::CameraScope inner(dialogueCamera,{0x2000,99,0.08f});Assert(dialogueCamera.actor==99);}Assert(dialogueCamera.actor==82);throw 1;}catch(int){}
 Assert(dialogueCamera.camera==0);
 Assert(positionCalls==604 && lookCalls==601);
 rendererViewOriginal=&ViewMock;
 alignas(16) heels::Matrix view{1,0,0,10,0,1,0,20,0,0,1,30,0,0,0,1};const auto savedView=view;
 Assert(RendererView(camera,0,view.data())==0xabcd1234u);Assert(viewPointer==view.data() && viewValue==view && view==savedView);
 stageState={0x5000,GetTickCount64()+1000,82,99,true};
 Assert(RendererView(camera,0,view.data())==0xabcd1234u);Assert(viewPointer==view.data() && viewValue==view && view==savedView);
 stageState={};Assert(viewCalls==2);
 std::array<std::byte,0x320> mode{};
 const auto modePointer=reinterpret_cast<std::uintptr_t>(mode.data());
 const std::uint8_t scripted=1;const std::uint32_t focus=82;
 std::memcpy(mode.data()+0x20c,&scripted,sizeof(scripted));std::memcpy(mode.data()+0x16c,&focus,sizeof(focus));
 dialogBeginOriginal=&BeginMock;dialogEndOriginal=&EndMock;
 stageState={modePointer,GetTickCount64()+1000,82,99,true,true};
 Assert(DialogBegin(mode.data(),0)==0x11223344u);Assert(stageState.mode==modePointer && !stageState.ending && stageState.focus==82);
 Assert(DialogEnd(mode.data(),0,7)==0x55667788u);Assert(stageState.ending);
 CaptureStage(mode.data());Assert(stageState.ending); // An outer Update must not resurrect a closed dialogue.
 Assert(DialogBegin(mode.data(),0)==0x11223344u);Assert(!stageState.ending);stageState={};
 std::cout<<"PASS: "<<checks<<" production camera detour checks; scope isolation, forwarding, native inputs preserved, position/look pair, no accumulation and exception restoration\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
