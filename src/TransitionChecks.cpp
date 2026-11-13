#include "Probe.cpp"
#include <iostream>
#include <stdexcept>
#include <source_location>
namespace {
unsigned checks=0;void AssertTransition(bool b,const std::source_location loc=std::source_location::current()){++checks;if(!b)throw std::runtime_error("model transition callback check failed at line "+std::to_string(loc.line()));}
std::unordered_map<unsigned,std::uintptr_t> objects;
std::uintptr_t appPointer=0x1111;
std::uintptr_t __fastcall LookupMock(void*,int,std::uint32_t id){const auto it=objects.find(id);return it==objects.end()?0:it->second;}
template<class T>void StoreTransition(std::uintptr_t at,const T& value){std::memcpy(reinterpret_cast<void*>(at),&value,sizeof(value));}
struct TransitionFixture {
 std::array<std::byte,0x1000> creature{};std::array<std::byte,0x100> set{};std::array<std::byte,16> holder{};
 std::uintptr_t Address()const{return reinterpret_cast<std::uintptr_t>(creature.data());}
 void ModelIs(std::uintptr_t model){StoreTransition(reinterpret_cast<std::uintptr_t>(holder.data())+4,model);}
 void Init(unsigned id,std::uintptr_t model){
  StoreTransition(Address()+0x10,id);StoreTransition(Address()+0x254,std::uint32_t{20});
  StoreTransition(Address()+0x120,std::array<float,4>{10,20,30,1});
  StoreTransition(Address()+0x154,reinterpret_cast<std::uintptr_t>(set.data()));
  StoreTransition(reinterpret_cast<std::uintptr_t>(set.data())+0x18,reinterpret_cast<std::uintptr_t>(holder.data()));
  ModelIs(model);objects[id]=Address();
 }
};
}
namespace {
std::array<std::byte,0x110> fakeCamera{},fakeScene{},fakeWrapper{};
std::array<std::uintptr_t,10> graphicsVtable{};std::uintptr_t graphicsObject=0,graphicsPointer=0;
std::unordered_map<unsigned,std::array<float,4>> heads;
unsigned scriptedWrites=0,scriptedReads=0,headGetterCalls=0;
std::uintptr_t __fastcall SceneMock(void*,int){return reinterpret_cast<std::uintptr_t>(fakeScene.data());}
bool __fastcall HeadMock(void* creature,int,void* out){++headGetterCalls;unsigned id=0;std::memcpy(&id,static_cast<std::byte*>(creature)+0x10,4);const auto it=heads.find(id);if(it==heads.end())return false;std::memcpy(out,it->second.data(),16);return true;}
std::uintptr_t __fastcall ScriptViewMock(void* camera,int,const void* matrix){++scriptedWrites;heels::Matrix view{},world{};std::memcpy(view.data(),matrix,64);AssertTransition(heels::CameraWorld(view,world));std::memcpy(static_cast<std::byte*>(camera)+0x80,view.data(),64);std::memcpy(static_cast<std::byte*>(camera)+0xc0,world.data(),64);return 0x12345678;}
std::uintptr_t __fastcall ScriptReadMock(void* camera,int,void* matrix){++scriptedReads;std::memcpy(matrix,static_cast<std::byte*>(camera)+0x80,64);return reinterpret_cast<std::uintptr_t>(matrix);}
}
namespace {
std::array<std::byte,0x100> helmetEquipment{};std::array<std::byte,0x600> helmetItem{};
std::uintptr_t __fastcall HelmetEquippedMock(void* object,int,const void* data){const auto* key=static_cast<const std::int32_t*>(data);if(object==helmetEquipment.data() && key[2]==5)return reinterpret_cast<std::uintptr_t>(helmetItem.data());return 0;}
unsigned helmetTransferCalls=0;bool nativeMaskArgsOK=false;
std::uintptr_t nativeExpectedScene=0,nativeExpectedAppearance=0;
void __fastcall HelmetTransferMock(void* appearance,int,std::uint32_t slot,void* item){
 ++helmetTransferCalls;AssertTransition(slot==5);std::uint16_t base=0;std::uint8_t variation=0;Read(reinterpret_cast<std::uintptr_t>(item)+0x464,base);Read(reinterpret_cast<std::uintptr_t>(item)+0x466,variation);StoreTransition(reinterpret_cast<std::uintptr_t>(appearance)+0x134,base);StoreTransition(reinterpret_cast<std::uintptr_t>(appearance)+0x13d,variation);
}
extern "C" {alignas(16) float EA_TestVector[4] __asm__("EA_TestVector")={1,2,3,4};}
extern "C" void __cdecl EA_TestNativeBody(void*,int,void*) __asm__("EA_TestNativeBody");
extern "C" void __cdecl EA_TestNativeBody(void* self,int edx,void* appearance){
 nativeMaskArgsOK=reinterpret_cast<std::uintptr_t>(self)==nativeExpectedScene && reinterpret_cast<std::uintptr_t>(appearance)==nativeExpectedAppearance && edx==17;
 StoreTransition(reinterpret_cast<std::uintptr_t>(appearance)+0x134,std::uint16_t{0xffff});StoreTransition(reinterpret_cast<std::uintptr_t>(appearance)+0x13d,std::uint8_t{255});
}
__attribute__((naked,noinline)) std::uintptr_t __fastcall NativeMaskMock(void*,int,void*){
 __asm__ volatile("pushl %ebp\n\tmovl %esp,%ebp\n\tandl $-16,%esp\n\tleal -4(%esp),%esp\n\tpushl 8(%ebp)\n\tpushl %edx\n\tpushl %ecx\n\tcall EA_TestNativeBody\n\tmovl %ebp,%esp\n\tpopl %ebp\n\tmovups EA_TestVector,%xmm0\n\tmovl $0x11223344,%eax\n\tmovl $0x12345678,%ecx\n\tmovl $0x55667788,%edx\n\tstc\n\tret $4\n\t");
}
struct RegisterCapture {std::uint32_t eax=0,ecx=0,edx=0,flags=0;float xmm0[4]{};};
__attribute__((naked,noinline)) void __cdecl RunMaskBridge(void*,void*,RegisterCapture*){
 __asm__ volatile("pushl %ebp\n\tmovl %esp,%ebp\n\tpushl %ebx\n\tpushl %esi\n\tpushl %edi\n\tmovl 16(%ebp),%edi\n\tpushl 12(%ebp)\n\tmovl 8(%ebp),%ecx\n\tmovl $17,%edx\n\tcall EA_HelmetSceneMask\n\tmovl %eax,0(%edi)\n\tmovl %ecx,4(%edi)\n\tmovl %edx,8(%edi)\n\tpushfl\n\tpopl %eax\n\tmovl %eax,12(%edi)\n\tmovups %xmm0,16(%edi)\n\tpopl %edi\n\tpopl %esi\n\tpopl %ebx\n\tleave\n\tret\n\t");
}
}

namespace {
std::array<std::byte,64> savedNameNode{};std::array<std::uintptr_t,1> savedNameArray{};
iv::NativeVector savedNamePool{};std::uintptr_t savedVmPool=0,savedVm=0;
iv::IntegerVariable savedVariable{};unsigned savedWrites=0,partyRebuilds=0,expectedNativeCreate=1;
void* __fastcall SavedInternMock(void* key,int,void* pool,const wchar_t* name,unsigned create){
 AssertTransition(reinterpret_cast<std::uintptr_t>(pool)==0 && std::wstring_view(name)==helmetChoiceName && create==1);
 const std::array<std::uint32_t,2> value{0,0};std::memcpy(key,value.data(),8);return key;
}
void __fastcall SavedSetterMock(void* pool,int,const void* key,int value,unsigned create,unsigned dirty){
 AssertTransition(create==expectedNativeCreate && dirty==1 && (value==0 || value==1));const auto* k=static_cast<const std::uint32_t*>(key);AssertTransition(k[0]==0 && k[1]==0);++savedWrites;
 savedVariable={0,0,1,value,dirty};const auto at=reinterpret_cast<std::uintptr_t>(&savedVariable);
 StoreTransition(reinterpret_cast<std::uintptr_t>(pool)+16,iv::NativeVector{0,static_cast<unsigned>(at),static_cast<unsigned>(at+20),static_cast<unsigned>(at+20)});
}
bool __fastcall PartyRebuildMock(void* clone,int,void* appearance,unsigned a,unsigned b,unsigned c){
 AssertTransition(a==0 && b==0 && c==0 && clone!=nullptr);++partyRebuilds;unsigned id=0;Read(reinterpret_cast<std::uintptr_t>(clone)+0x10,id);return HelmetBuildProbe(appearance,nullptr,0,id)==0xaabbccdd;
}
}

namespace {
double forwardedWeaponFactor=1;const void* expectedNativeWeaponMatrix=nullptr;unsigned weaponNodeForwards=0;
void __fastcall WeaponNodeForwardMock(void*,int,const void* parent,const void* model,const void* inverse){
 ++weaponNodeForwards;heels::Matrix p{},m{},inv{};std::memcpy(p.data(),parent,64);std::memcpy(m.data(),model,64);std::memcpy(inv.data(),inverse,64);
 AssertTransition(std::abs(p[0]-forwardedWeaponFactor)<.00001 && m[0]==1);
 AssertTransition(inv[0]==1);
 AssertTransition(model==expectedNativeWeaponMatrix);
}
bool checkPreparedWeapon=false;unsigned coldWeaponReads=0;
std::vector<std::byte> coldWeaponMMH,coldWeaponMSH{std::byte{1},std::byte{2},std::byte{3}};
std::optional<std::vector<std::byte>> ColdWeaponReadMock(std::wstring_view name){++coldWeaponReads;if(name.ends_with(L".mmh"))return coldWeaponMMH;if(name.ends_with(L".msh"))return coldWeaponMSH;return {};}
std::optional<std::wstring> ColdWeaponGdaMock(std::int32_t table,std::int32_t,std::uint32_t column){
 if(table==92)return L"M";if(column==dao::core::GdaColumnHash("ModelType"))return L"P";return L"H";
}
std::uintptr_t expectedWeaponWield=0,expectedWeaponName=0,expectedWeaponParts=0,expectedWeaponAppearance=0;
std::uintptr_t __cdecl WeaponLoadMock(void* w,const void* n,const void* parts,unsigned slot,unsigned extra,unsigned flag){
 AssertTransition(reinterpret_cast<std::uintptr_t>(w)==expectedWeaponWield && reinterpret_cast<std::uintptr_t>(n)==expectedWeaponName && reinterpret_cast<std::uintptr_t>(parts)==expectedWeaponParts && slot==3 && extra==123 && flag==1);return 0x13579b01;
}
std::uintptr_t __cdecl WeaponAttachMock(void* w,void* appearance,unsigned slot,unsigned hand){
 AssertTransition(reinterpret_cast<std::uintptr_t>(w)==expectedWeaponWield && reinterpret_cast<std::uintptr_t>(appearance)==expectedWeaponAppearance && slot==3 && hand==2);
 if(checkPreparedWeapon){const auto& b=weaponBindings.at(WeaponProbeModel(expectedWeaponWield,3));AssertTransition(b.attached && b.ready && std::abs(b.factor-.85)<1e-12);}
 return 0x2468ac01;
}
}
namespace dao::Module {
std::uintptr_t Address(AddressId id){switch(static_cast<unsigned>(id)){case 5504:return reinterpret_cast<std::uintptr_t>(&SavedInternMock);case 16032:return reinterpret_cast<std::uintptr_t>(&SavedSetterMock);case 100262:return savedVmPool;case 99736:return reinterpret_cast<std::uintptr_t>(&savedVm);case 14969:return reinterpret_cast<std::uintptr_t>(&PartyRebuildMock);case 100835:return reinterpret_cast<std::uintptr_t>(&appPointer);case 10509:return reinterpret_cast<std::uintptr_t>(&LookupMock);case 100485:return reinterpret_cast<std::uintptr_t>(&graphicsPointer);case 74603:return 0x12340000;case 17790:return reinterpret_cast<std::uintptr_t>(&HelmetTransferMock);case 68885:return 0x1111;case 68886:return 0x2222;case 68551:return 0x3333;case 19468:return reinterpret_cast<std::uintptr_t>(&HelmetEquippedMock);case 69083:return 0x12345000;case 69015:return 0x12346000;case 8310:return 0x55d760;case 9643:return 0x5b0f00;case 14447:return reinterpret_cast<std::uintptr_t>(&HeadMock);default:return 0;}}
std::uintptr_t Base(){return reinterpret_cast<std::uintptr_t>(&SceneMock)-0x1000;}
const daoaddr::Library* Library(){return nullptr;}
std::string Instance(){return {};}
std::filesystem::path OwnFolder(){return {};}
std::optional<core::Fingerprint> Fingerprint(){return std::nullopt;}
}
namespace {
void SampleComputedHead(const heels::Actor& actor,const std::array<float,4>& native,float h){
 std::array<std::byte,0x200> node{},definition{};std::array<std::uintptr_t,24> vtable{};vtable[10]=0x55d760;
 const auto np=reinterpret_cast<std::uintptr_t>(node.data()),dp=reinterpret_cast<std::uintptr_t>(definition.data());
 StoreTransition(np,reinterpret_cast<std::uintptr_t>(vtable.data()));StoreTransition(np+0x154,actor.model-8);StoreTransition(np+0x174,dp);
 dao::core::WStringRep name{};name.size=4;name.capacity=7;std::memcpy(name.data.buffer,L"head",8);StoreTransition(dp+0xc0,name);
 heels::Matrix world{1,0,0,native[0],0,1,0,native[1],0,0,1,native[2]+h,0,0,0,1};StoreTransition(np+0x60,world);
 HeadCapture capture{actor,{}};{HeadCaptureScope scope(&capture);CaptureComputedHead(node.data());}
 AssertTransition(capture.world.has_value());capture.world.reset();name.data.buffer[0]=L'H';StoreTransition(dp+0xc0,name);{HeadCaptureScope scope(&capture);CaptureComputedHead(node.data());}AssertTransition(capture.world.has_value());auto point=*capture.world;point[2]-=h;headSamples[actor.id]={actor,point,GetTickCount64()};
 // Rejected names/owners cannot populate another body's head cache.
 capture.world.reset();StoreTransition(np+0x154,std::uintptr_t{0});{HeadCaptureScope scope(&capture);CaptureComputedHead(node.data());}AssertTransition(!capture.world);
 StoreTransition(np+0x154,actor.model-8);name.data.buffer[0]=L'f';StoreTransition(dp+0xc0,name);{HeadCaptureScope scope(&capture);CaptureComputedHead(node.data());}AssertTransition(!capture.world);
 AssertTransition(headCapture==nullptr);
}
}
namespace {
unsigned previewTickCalls=0;float previewDt=0;void* pendingNode=nullptr;
std::uintptr_t __fastcall PreviewCreateMock(void* self,int){AssertTransition(previewCreation==reinterpret_cast<std::uintptr_t>(self));const auto value=FindPreview(pendingNode);AssertTransition(value && value->source.id==82);return 0x99887766;}
std::uintptr_t __fastcall PreviewTickMock(void*,int,float dt){++previewTickCalls;previewDt=dt;return 0xaabbccdd;}
}

extern "C" {alignas(16) std::uint32_t EA_BuildCapture[12] __asm__("EA_BuildCapture")={};}
__attribute__((used,naked,noinline)) std::uintptr_t __cdecl NativeBuildCapture(void*,void*,unsigned,unsigned){
 __asm__ volatile("movl %eax,EA_BuildCapture\n\tmovl %ecx,EA_BuildCapture+4\n\tmovl %edx,EA_BuildCapture+8\n\tpushfl\n\tpopl %eax\n\tmovl %eax,EA_BuildCapture+12\n\tmovups %xmm0,EA_BuildCapture+16\n\tmovl 4(%esp),%eax\n\tmovl %eax,EA_BuildCapture+32\n\tmovl 8(%esp),%eax\n\tmovl %eax,EA_BuildCapture+36\n\tmovl 12(%esp),%eax\n\tmovl %eax,EA_BuildCapture+40\n\tmovl 16(%esp),%eax\n\tmovl %eax,EA_BuildCapture+44\n\tmovl $0x11223344,%eax\n\tret\n\t");
}
__attribute__((used,naked,noinline)) std::uintptr_t __cdecl ExerciseBuildProbe(){
 __asm__ volatile("pushl %ebp\n\tmovl %esp,%ebp\n\tandl $-16,%esp\n\tpushl $-1\n\tpushl $37\n\tpushl $0x5678\n\tpushl $0\n\tmovl $111,%eax\n\tmovl $222,%ecx\n\tmovl $333,%edx\n\tmovups EA_TestVector,%xmm0\n\tstc\n\tcall EA_HelmetBuildProbe\n\tleal 16(%esp),%esp\n\tleave\n\tret\n\t");
}
bool observedHidden=false;
std::uintptr_t __cdecl BuildVisibilityMock(void* appearance,void*,unsigned,unsigned){
 VisualHelmetFields f;ReadVisualHelmet(reinterpret_cast<std::uintptr_t>(appearance),f);
 observedHidden=f.base==0xffff && f.variation==255 && f.fullFace==0;return 0xaabbccdd;
}
void CheckBuildProbe(){
 EA_BuildOriginal=&NativeBuildCapture;
 for(unsigned n=0;n<1000;++n){
  AssertTransition(ExerciseBuildProbe()==0x11223344);
  AssertTransition(EA_BuildCapture[0]==111 && EA_BuildCapture[1]==222 && EA_BuildCapture[2]==333 && (EA_BuildCapture[3]&1));
  AssertTransition(std::memcmp(EA_BuildCapture+4,EA_TestVector,16)==0);
  AssertTransition(EA_BuildCapture[8]==0 && EA_BuildCapture[9]==0x5678 && EA_BuildCapture[10]==37 && EA_BuildCapture[11]==0xffffffff);
 }
}

int main(int argc,char** argv){try{
 CheckBuildProbe();
 inventoryPreview=true; // Exercise the optional experimental path as well.
 spdlog::set_level(spdlog::level::off);actors=heels::Actors{};
 TransitionFixture morrigan,hero;morrigan.Init(100,0x1000);hero.Init(82,0x2000);
 const auto m=Identify(morrigan.Address()),h=Identify(hero.Address());AssertTransition(m.has_value() && h.has_value());Track(*m);Track(*h);
 actors.Rendered(*m,0.08f,100);actors.Rendered(*h,0.05f,100);
 heels::Matrix world{1,0,0,10,0,1,0,20,0,0,1,30,0,0,0,1};
 // Recreate the dialogue model replacement, without waiting for periodic Refresh.
 morrigan.ModelIs(0x3000);AssertTransition(!actors.Find(0x3000));
 const auto fresh=ResolveModel(0x3000,world);AssertTransition(fresh && fresh->id==100 && fresh->creature==morrigan.Address());
 AssertTransition(!actors.Find(0x1000));AssertTransition(actors.Find(0x2000)==h);
 actors.Rendered(*fresh,0.08f,101);
 morrigan.ModelIs(0x1000);const auto restored=ResolveModel(0x1000,world);
 AssertTransition(restored && restored->id==100);AssertTransition(!actors.Find(0x3000));
 AssertTransition(actors.RenderedHeight(*h,102)==0.05f);
 actors.Rendered(*restored,0.08f,102);
 // An early binding attempt at an initialization pose must not suppress the later live pose.
 AssertTransition(!ResolveModel(0x4000,world));morrigan.ModelIs(0x4000);world[11]=31;StoreTransition(morrigan.Address()+0x128,31.f);
 const auto initialized=ResolveModel(0x4000,world);AssertTransition(initialized && initialized->id==100);
 actors.Rendered(*initialized,0.08f,103);
 AssertTransition(!ResolveModel(0x4500,world));morrigan.ModelIs(0x4500);
 dialogueTransitionUntil.store(GetTickCount64()+1000);
 const auto samePose=ResolveModel(0x4500,world);AssertTransition(samePose && samePose->id==100);
 actors.Rendered(*samePose,0.08f,104);objects.erase(100);morrigan.ModelIs(0x5000);
 AssertTransition(!ResolveModel(0x5000,world));AssertTransition(actors.Find(0x2000)==h);

 // Scripted-camera production path, with native engine APIs replaced by local fixtures.
 morrigan.Init(100,0x1000);hero.Init(82,0x2000);actors=heels::Actors{};const auto ma=Identify(morrigan.Address()),ha=Identify(hero.Address());Track(*ma);Track(*ha);
 const auto now=GetTickCount64();actors.Rendered(*ma,0.08f,now,{0,4,0,1});actors.Rendered(*ha,0.05f,now,{2,1,0,1});
 SampleComputedHead(*ma,{0,4,1.5f,1},0.08f);SampleComputedHead(*ha,{2,1,1.5f,1},0.05f);
 graphicsVtable[9]=reinterpret_cast<std::uintptr_t>(&SceneMock);graphicsObject=reinterpret_cast<std::uintptr_t>(graphicsVtable.data());graphicsPointer=reinterpret_cast<std::uintptr_t>(&graphicsObject);
 const auto cp=reinterpret_cast<std::uintptr_t>(fakeCamera.data());StoreTransition(cp,std::uintptr_t{0x12340000});
 StoreTransition(reinterpret_cast<std::uintptr_t>(fakeScene.data())+0xc0,reinterpret_cast<std::uintptr_t>(fakeWrapper.data()));StoreTransition(reinterpret_cast<std::uintptr_t>(fakeWrapper.data())+4,cp);
 dialogueThread=GetCurrentThreadId();stageState={0x5000,now+100000,100,82,true};active=true;dialogueCompensation=true;
 rendererViewOriginal=&ScriptViewMock;rendererReadOriginal=&ScriptReadMock;
 // Camera looking along +Y, exactly toward Morrigan's native head.
 heels::Matrix nativeView{1,0,0,0,0,0,1,-1.5f,0,-1,0,0,0,0,0,1},out{};const auto saved=nativeView;
 AssertTransition(MainCamera()==cp);AssertTransition(RendererView(fakeCamera.data(),0,nativeView.data())==0x12345678);
 for(unsigned i=0;i<1000;++i){AssertTransition(RendererRead(fakeCamera.data(),0,out.data())==reinterpret_cast<std::uintptr_t>(out.data()));AssertTransition(heels::NearMatrix(out,heels::RaisedView(nativeView,0.08f)));}
 AssertTransition(scriptedReads==1000);AssertTransition(nativeView==saved);AssertTransition(scriptedWrites==2); // Native write plus exactly one correction.
 active=false;RendererRead(fakeCamera.data(),0,out.data());AssertTransition(heels::NearMatrix(out,nativeView));active=true;
 RendererRead(fakeCamera.data(),0,out.data());AssertTransition(heels::NearMatrix(out,heels::RaisedView(nativeView,0.08f)));
 RestoreMainCamera();Read(cp+0x80,out);AssertTransition(heels::NearMatrix(out,nativeView));
 // An expired scene restores the unmodified native baseline.
 RendererRead(fakeCamera.data(),0,out.data());stageState.expires=0;RendererRead(fakeCamera.data(),0,out.data());AssertTransition(heels::NearMatrix(out,nativeView));
 // Fresh native writes replace the baseline rather than adding to an old correction.
 stageState.expires=now+100000;nativeView[3]=0.1f;RendererView(fakeCamera.data(),0,nativeView.data());RendererRead(fakeCamera.data(),0,out.data());AssertTransition(heels::NearMatrix(out,heels::RaisedView(nativeView,0.08f)));
 // An invalid head does not borrow the other participant's height.
 headSamples.erase(100);RendererRead(fakeCamera.data(),0,out.data());AssertTransition(heels::NearMatrix(out,nativeView));
 SampleComputedHead(*ma,{0,4,1.5f,1},0.08f);
 // A different scene camera is never touched.
 StoreTransition(reinterpret_cast<std::uintptr_t>(fakeWrapper.data())+4,std::uintptr_t{0});const auto writesBefore=scriptedWrites;RendererRead(fakeCamera.data(),0,out.data());AssertTransition(scriptedWrites==writesBefore);
 stageState={};AssertTransition(headGetterCalls==0);

 // GUI previews: identify source through native owner fields, never clone ID or position.
 TransitionFixture clone;clone.Init(999,0x3000);objects.erase(999);
 StoreTransition(hero.Address(),std::uintptr_t{0xfeed1000});StoreTransition(clone.Address(),std::uintptr_t{0xfeed1000});
 std::array<std::byte,0x320> paper{};const auto pp=reinterpret_cast<std::uintptr_t>(paper.data());
 StoreTransition(pp,std::uintptr_t{0x12345000});StoreTransition(pp+0x58,hero.Address());StoreTransition(pp+0x68,clone.Address());
 std::array<std::byte,0x200> previewNode{};const auto pn=reinterpret_cast<std::uintptr_t>(previewNode.data());StoreTransition(pn+0x154,std::uintptr_t{0x3000-8});
 paperTickOriginal=&PreviewTickMock;partyTickOriginal=&PreviewTickMock;
 const auto worldActors=actors.Size();AssertTransition(PaperTick(paper.data(),7,.125f)==0xaabbccdd);AssertTransition(previewDt==.125f && previewTickCalls==1);
 const auto binding=FindPreview(previewNode.data());AssertTransition(binding && binding->source.id==82 && binding->source.creature==hero.Address() && binding->clone==clone.Address());AssertTransition(actors.Size()==worldActors);
 // A dependent child node cannot receive a second independent displacement.
 StoreTransition(pn+0x15c,std::uintptr_t{1});AssertTransition(!FindPreview(previewNode.data()));StoreTransition(pn+0x15c,std::uintptr_t{0});
 StoreTransition(pp+0x68,std::uintptr_t{0});AssertTransition(!FindPreview(previewNode.data()));StoreTransition(pp+0x68,clone.Address());
 previews[0x3000].at=0;AssertTransition(!FindPreview(previewNode.data()));ObservePreview(PreviewKind::Paperdoll,paper.data());AssertTransition(FindPreview(previewNode.data()).has_value());
 clone.ModelIs(0x4000);AssertTransition(!FindPreview(previewNode.data()));
 StoreTransition(pn+0x154,std::uintptr_t{0x4000-8});AssertTransition(FindPreview(previewNode.data()).has_value()); // No GUI tick needed.
 AssertTransition(!previews.contains(0x3000));
 // Replacing the entire preview creature is also resolved before its first draw.
 TransitionFixture replacement;replacement.Init(998,0x5000);objects.erase(998);StoreTransition(replacement.Address(),std::uintptr_t{0xfeed1000});
 StoreTransition(pp+0x68,replacement.Address());StoreTransition(pn+0x154,std::uintptr_t{0x5000-8});
 const auto replaced=FindPreview(previewNode.data());AssertTransition(replaced && replaced->clone==replacement.Address() && replaced->source.id==82);
 StoreTransition(pp+0x68,clone.Address());StoreTransition(pn+0x154,std::uintptr_t{0x4000-8});AssertTransition(FindPreview(previewNode.data()).has_value());
 PaperTick(paper.data(),0,.25f);
 // Party picker entries bind an original object ID to a separate graphical creature.
 previews.clear(); // Separate GUI scenes own distinct clones in the game.
 std::array<std::byte,0x34> partyEntry{};const auto pe=reinterpret_cast<std::uintptr_t>(partyEntry.data());StoreTransition(pe,std::uint32_t{82});StoreTransition(pe+4,clone.Address());
 AssertTransition(PartyTick(partyEntry.data(),0,.5f)==0xaabbccdd);AssertTransition(previewDt==.5f);const auto partyBinding=FindPreview(previewNode.data());AssertTransition(partyBinding && partyBinding->kind==PreviewKind::PartyPicker && partyBinding->source.id==82);
 objects.erase(82);AssertTransition(!FindPreview(previewNode.data()));objects[82]=hero.Address();
 StoreTransition(pe+4,hero.Address());AssertTransition(!FindPreview(previewNode.data())); // World actor itself is not a preview clone.
 // A new model is updated before the GUI publishes its clone pointer.
 previews.clear();StoreTransition(pp+0x68,std::uintptr_t{0});
 std::array<std::byte,0x210> command{};const auto commandPtr=reinterpret_cast<std::uintptr_t>(command.data());
 StoreTransition(commandPtr,std::uintptr_t{0x12346000});StoreTransition(commandPtr+8,std::uint32_t{42});StoreTransition(pp+0x2f0,std::uint32_t{42});
 StoreTransition(commandPtr+0x208,pp);StoreTransition(commandPtr+0x204,hero.Address());StoreTransition(commandPtr+0x200,clone.Address());
 AssertTransition(!FindPreview(previewNode.data()));
 {PreviewCreationScope scope(commandPtr);const auto first=FindPreview(previewNode.data());AssertTransition(first && first->clone==clone.Address() && first->source.id==82);
  StoreTransition(pp+0x2f0,std::uint32_t{43});AssertTransition(!FindPreview(previewNode.data()));StoreTransition(pp+0x2f0,std::uint32_t{42});
  StoreTransition(pp+0x58,morrigan.Address());AssertTransition(!FindPreview(previewNode.data()));StoreTransition(pp+0x58,hero.Address());
  StoreTransition(commandPtr+0x200,hero.Address());AssertTransition(!FindPreview(previewNode.data()));StoreTransition(commandPtr+0x200,clone.Address());
 }
 AssertTransition(previewCreation==0);AssertTransition(!FindPreview(previewNode.data()));
 pendingNode=previewNode.data();previewCreateOriginal=&PreviewCreateMock;AssertTransition(PreviewCreate(command.data(),7)==0x99887766);AssertTransition(previewCreation==0);
 try{PreviewCreationScope scope(commandPtr);throw 1;}catch(int){}AssertTransition(previewCreation==0);
 // Release default: inventory paths are excluded, including creation scope.
 inventoryPreview=false;previews.clear();
 StoreTransition(pp+0x68,clone.Address());
 AssertTransition(!ReadPreview(PreviewKind::Paperdoll,pp));
 {PreviewCreationScope scope(commandPtr);AssertTransition(!FindPreview(previewNode.data()));}
 ObservePreview(PreviewKind::Paperdoll,paper.data());AssertTransition(previews.empty());
 // Party selection remains enabled with inventory lifting disabled.
 StoreTransition(pe+4,clone.Address());ObservePreview(PreviewKind::PartyPicker,partyEntry.data());
 const auto retainedParty=FindPreview(previewNode.data());AssertTransition(retainedParty && retainedParty->kind==PreviewKind::PartyPicker && retainedParty->source.id==82);
 // Read-only helmet snapshots; no native getter detours or state writes.
 std::array<std::byte,0x200> helmetAppearance{},cloneAppearance{};
 const auto hp=reinterpret_cast<std::uintptr_t>(helmetAppearance.data()),chp=reinterpret_cast<std::uintptr_t>(cloneAppearance.data());
 StoreTransition(hero.Address()+0x7b0,hp);StoreTransition(clone.Address()+0x7b0,chp);
 StoreTransition(hp+0x58,std::uint16_t{15});StoreTransition(hp+0x134,std::uint16_t{20});StoreTransition(hp+0x13d,std::uint8_t{52});StoreTransition(hp+0x5e,std::uint8_t{1});
 StoreTransition(chp+0x58,std::uint16_t{15});StoreTransition(chp+0x134,std::uint16_t{20});StoreTransition(chp+0x13d,std::uint8_t{52});
 const auto originalHero=hero.creature,originalClone=clone.creature;const auto originalAppearance=helmetAppearance,originalCloneAppearance=cloneAppearance;
 HelmetSnapshot snapshot;AssertTransition(ReadHelmetSnapshot(hero.Address(),hero.Address(),snapshot));AssertTransition(snapshot.actor==82 && snapshot.base==20 && snapshot.variation==52 && snapshot.fullFace==1 && snapshot.clone==0);
 AssertTransition(ReadHelmetSnapshot(hero.Address(),clone.Address(),snapshot));AssertTransition(snapshot.clone==clone.Address() && snapshot.source==hero.Address());
 AssertTransition(!ReadHelmetSnapshot(0,clone.Address(),snapshot));AssertTransition(!ReadHelmetSnapshot(hero.Address(),0x2000,snapshot));
 ReportHelmetSnapshot("world",hero.Address(),hero.Address());ReportHelmetSnapshot("inventory",hero.Address(),clone.Address());
 AssertTransition(hero.creature==originalHero && clone.creature==originalClone && helmetAppearance==originalAppearance && cloneAppearance==originalCloneAppearance);
 // Native filtering remains untouched; restore only the missing helmet afterwards.
 std::array<std::byte,0x600> scene{},helmetAppearanceOut{};const auto scenePtr=reinterpret_cast<std::uintptr_t>(scene.data()),outPtr=reinterpret_cast<std::uintptr_t>(helmetAppearanceOut.data());
 StoreTransition(hero.Address()+0xbec,reinterpret_cast<std::uintptr_t>(helmetEquipment.data()));StoreTransition(reinterpret_cast<std::uintptr_t>(helmetEquipment.data()),std::uintptr_t{0x1111});StoreTransition(reinterpret_cast<std::uintptr_t>(helmetEquipment.data())+0x24,std::uintptr_t{0x2222});
 StoreTransition(reinterpret_cast<std::uintptr_t>(helmetItem.data()),std::uintptr_t{0x3333});StoreTransition(reinterpret_cast<std::uintptr_t>(helmetItem.data())+0x4a8,std::uint32_t{82});StoreTransition(reinterpret_cast<std::uintptr_t>(helmetItem.data())+0x464,std::uint16_t{47});StoreTransition(reinterpret_cast<std::uintptr_t>(helmetItem.data())+0x466,std::uint8_t{2});
 StoreTransition(scenePtr+0x1c0,std::uint32_t{82});StoreTransition(scenePtr+0x4f0,std::uint8_t{0x37});stageState={0x5000,GetTickCount64()+100000,100,82,true};showHelmetDialog=true;
 const auto heroBeforeMask=hero.creature;const auto itemBeforeMask=helmetItem;const auto sceneBeforeMask=scene;
 EA_MaskOriginal=&NativeMaskMock;nativeExpectedScene=scenePtr;nativeExpectedAppearance=outPtr;
 for(unsigned repeat=0;repeat<1000;++repeat){
  RegisterCapture registers;RunMaskBridge(scene.data(),helmetAppearanceOut.data(),&registers);
  AssertTransition(nativeMaskArgsOK);AssertTransition(registers.eax==0x11223344 && registers.ecx==0x12345678 && registers.edx==0x55667788);AssertTransition((registers.flags&1)!=0);AssertTransition(registers.xmm0[0]==1 && registers.xmm0[1]==2 && registers.xmm0[2]==3 && registers.xmm0[3]==4);
  std::uint16_t restoredBase=0;std::uint8_t restoredVar=0;Read(outPtr+0x134,restoredBase);Read(outPtr+0x13d,restoredVar);AssertTransition(restoredBase==47 && restoredVar==2);AssertTransition(helmetLeaseDepth==0);
 }
 AssertTransition(helmetTransferCalls==1000);AssertTransition(hero.creature==heroBeforeMask && helmetItem==itemBeforeMask && scene==sceneBeforeMask);
 auto noRestore=[&](){const auto before=helmetTransferCalls;RegisterCapture registers;RunMaskBridge(scene.data(),helmetAppearanceOut.data(),&registers);AssertTransition(helmetTransferCalls==before);std::uint16_t base=0;Read(outPtr+0x134,base);AssertTransition(base==0xffff && helmetLeaseDepth==0);};
 showHelmetDialog=false;noRestore();showHelmetDialog=true;
 // Native scene filtering can run between dialogue modes, after expiry, or
 // for an actor outside the current hero/focus pair. Restore before build.
 auto mustRestore=[&](){const auto before=helmetTransferCalls;RegisterCapture registers;RunMaskBridge(scene.data(),helmetAppearanceOut.data(),&registers);VisualHelmetFields f;AssertTransition(ReadVisualHelmet(outPtr,f) && f.base==47 && f.variation==2 && helmetTransferCalls==before+1 && helmetLeaseDepth==0);};
 stageState.ending=true;mustRestore();stageState.ending=false;stageState.expires=0;mustRestore();stageState={};mustRestore();stageState={0x5000,GetTickCount64()+100000,100,101,true};mustRestore();
 AssertTransition(hero.creature==heroBeforeMask && helmetItem==itemBeforeMask && scene==sceneBeforeMask);
 StoreTransition(scenePtr+0x1c0,std::uint32_t{999});noRestore();StoreTransition(scenePtr+0x1c0,std::uint32_t{82});
 StoreTransition(reinterpret_cast<std::uintptr_t>(helmetItem.data())+0x4a8,std::uint32_t{999});noRestore();StoreTransition(reinterpret_cast<std::uintptr_t>(helmetItem.data())+0x4a8,std::uint32_t{82});

 hiddenHelmetActors.insert(82);
 // Hidden dialogues must not invoke the visible-helmet EV/native transfer.
 const auto beforeHidden=helmetTransferCalls;RegisterCapture hiddenRegs;
 RunMaskBridge(scene.data(),helmetAppearanceOut.data(),&hiddenRegs);
 VisualHelmetFields hiddenFields;ReadVisualHelmet(outPtr,hiddenFields);
 AssertTransition(hiddenFields.base==0xffff && hiddenFields.variation==255 && hiddenFields.fullFace==0);
 AssertTransition(helmetTransferCalls==beforeHidden && helmetLeaseDepth==0);
 // Exercise actual build bridge, not just a policy helper. All source and item
 // bytes, including the equipped-slot data, must survive each suppression.
 EA_BuildOriginal=&BuildVisibilityMock;
 for(unsigned repeat=0;repeat<1000;++repeat){
  const auto visualBefore=helmetAppearance;observedHidden=false;
  AssertTransition(HelmetBuildProbe(helmetAppearance.data(),nullptr,0,82)==0xaabbccdd);
  AssertTransition(observedHidden && buildHelmetDepth==0);
  AssertTransition(helmetAppearance==visualBefore && hero.creature==heroBeforeMask && helmetItem==itemBeforeMask);
 }
 // A copied inventory/unknown identity must not inherit the source actor toggle.
 observedHidden=true;const auto cloneBefore=cloneAppearance;
 HelmetBuildProbe(cloneAppearance.data(),nullptr,0,0x27710);
 AssertTransition(!observedHidden && cloneAppearance==cloneBefore && buildHelmetDepth==0);
 hiddenHelmetActors.clear();observedHidden=true;
 HelmetBuildProbe(helmetAppearance.data(),nullptr,0,82);
 AssertTransition(!observedHidden && buildHelmetDepth==0);

 // Persistent choice uses the native typed setter and a bounded, read-only pool reader.
 savedNameArray[0]=reinterpret_cast<std::uintptr_t>(savedNameNode.data());const auto namesAt=reinterpret_cast<std::uintptr_t>(savedNameArray.data());
 savedNamePool={0,static_cast<unsigned>(namesAt),static_cast<unsigned>(namesAt+4),static_cast<unsigned>(namesAt+4)};
 const auto name=dao::core::WStringRep::Borrow(helmetChoiceName);StoreTransition(savedNameArray[0]+12,name);
 savedVmPool=reinterpret_cast<std::uintptr_t>(&savedNamePool);savedVm=reinterpret_cast<std::uintptr_t>(&savedVmPool);
 saveHelmetChoices=true;savedChoices.clear();
 AssertTransition(ReadSavedHelmetChoice(hero.Address())==false && !HelmetHidden(82));
 const auto headBeforePersistence=heels::Equipped(hero.Address(),5);
 const auto irrelevant=hero.creature;
 StoreTransition(hero.Address()+0x2e0+16*5,iv::NativeVector{0,9,1,0});
 AssertTransition(ReadSavedHelmetChoice(hero.Address())==false);
 hero.creature=irrelevant;

 for(unsigned repeat=0;repeat<500;++repeat){
  AssertTransition(EA_SaveHelmetPreferenceV1(82,true));EA_SetHelmetHiddenV1(82,true);AssertTransition(ReadSavedHelmetChoice(hero.Address())==true && HelmetHidden(82));
  AssertTransition(EA_SaveHelmetPreferenceV1(82,false));EA_SetHelmetHiddenV1(82,false);AssertTransition(ReadSavedHelmetChoice(hero.Address())==false && !HelmetHidden(82));
 }
 AssertTransition(savedWrites==1000 && heels::Equipped(hero.Address(),5)==headBeforePersistence);
 const auto validSaved=savedVariable;savedVariable.value=2;AssertTransition(!ReadSavedHelmetChoice(hero.Address()));savedVariable=validSaved;
 // Regression: no active VM is required for a quickbar click.
 savedVariable.type=1;const auto beforeSavedWrites=savedWrites;const auto oldVm=savedVm;savedVm=0;
 AssertTransition(EA_SaveHelmetPreferenceV1(82,true));savedVm=oldVm;AssertTransition(savedWrites==beforeSavedWrites+1);
 AssertTransition(!EA_SaveHelmetPreferenceV1(999,true));
 // Read the stored value with no session ID cache, as after loading a save.
 AssertTransition(EA_SaveHelmetPreferenceV1(82,true));savedChoices.clear();hiddenHelmetActors.clear();AssertTransition(HelmetHidden(82));
 // No late party rebuild: the initial native model build receives helmet metadata
 // before DAO copies VFX onto the clone. The source item and creature stay unchanged.
 const auto sourceBeforeParty=hero.creature;const auto itemBeforeParty=helmetItem;
 ClearVisualHelmet(chp);const auto cloneStripped=cloneAppearance;
 observedHidden=true;HelmetBuildProbe(cloneAppearance.data(),nullptr,0,0x27710);AssertTransition(observedHidden && cloneAppearance==cloneStripped);
 AssertTransition(EA_SaveHelmetPreferenceV1(82,false));EA_SetHelmetHiddenV1(82,false);
 {PartyHelmetScope scope({hero.Address(),82});observedHidden=true;HelmetBuildProbe(cloneAppearance.data(),nullptr,0,0x27710);AssertTransition(!observedHidden);}
 VisualHelmetFields initialParty;AssertTransition(ReadVisualHelmet(chp,initialParty) && initialParty.base==47 && initialParty.variation==2);
 AssertTransition(partyHelmetContext.source==0 && hero.creature==sourceBeforeParty && helmetItem==itemBeforeParty);
 AssertTransition(EA_SaveHelmetPreferenceV1(82,true));EA_SetHelmetHiddenV1(82,true);
 {PartyHelmetScope scope({hero.Address(),82});observedHidden=false;HelmetBuildProbe(cloneAppearance.data(),nullptr,0,0x27710);AssertTransition(observedHidden);}
 const auto beforeNoLate=partyRebuilds;EA_UpdatePartyHelmetsV1();ObservePartyHelmetBinding(nullptr);AssertTransition(partyRebuilds==beforeNoLate);
 // Native option OFF never enters the creation scope.
 partyPickerVisibility=false;partyCreateOriginal=+[](void*,int,void*) __attribute__((fastcall)) ->std::uintptr_t{AssertTransition(!partyHelmetContext.source);return 123;};
 AssertTransition(PartyCreate(nullptr,0,reinterpret_cast<void*>(hero.Address()))==123);partyPickerVisibility=true;
 partyCreateOriginal=+[](void*,int,void* source) __attribute__((fastcall)) ->std::uintptr_t{AssertTransition(partyHelmetContext.source==reinterpret_cast<std::uintptr_t>(source) && partyHelmetContext.id==82);return 124;};
 AssertTransition(PartyCreate(nullptr,0,reinterpret_cast<void*>(hero.Address()))==124 && !partyHelmetContext.source);
 // Regression: a saved value must override a still-live cache from the previous save
 // during the native build, without a second rebuild on the frame callback.
 AssertTransition(EA_SaveHelmetPreferenceV1(82,false));EA_SetHelmetHiddenV1(82,true);
 observedHidden=true;HelmetBuildProbe(helmetAppearance.data(),nullptr,0,82);AssertTransition(!observedHidden && !HelmetHidden(82));
 AssertTransition(EA_SaveHelmetPreferenceV1(82,true));EA_SetHelmetHiddenV1(82,false);
 observedHidden=false;const auto tintSourceBefore=hero.creature;const auto tintAppearanceBefore=helmetAppearance;
 const auto beforeRefreshRebuilds=partyRebuilds;
 HelmetBuildProbe(helmetAppearance.data(),nullptr,0,82);AssertTransition(observedHidden && HelmetHidden(82));
 for(unsigned tick=0;tick<100;++tick)EA_RefreshSavedHelmetsV1();
 AssertTransition(partyRebuilds==beforeRefreshRebuilds && hero.creature==tintSourceBefore && helmetAppearance==tintAppearanceBefore);
 // Real native load bridge: undeclared plugin variable may be created, all arguments forwarded.
 EA_SavedIntOriginal=&SavedSetterMock;
 const std::array<std::uint32_t,2> loadKey{0,0};const auto loadSource=hero.Address();
 SavedHelmetInt(reinterpret_cast<void*>(loadSource+0x2e0),0,loadKey.data(),1,0,1);
 AssertTransition(ReadSavedHelmetChoice(loadSource)==true);
 SavedHelmetInt(reinterpret_cast<void*>(loadSource+0x2e0),0,loadKey.data(),0,0,1);
 AssertTransition(ReadSavedHelmetChoice(loadSource)==false);
 AssertTransition(!EA_AllowSavedHelmetKey(0,0) && !EA_AllowSavedHelmetKey(reinterpret_cast<std::uintptr_t>(loadKey.data()),1));
 const auto originalNameNode=savedNameNode;const auto foreign=dao::core::WStringRep::Borrow(L"OTHER_VARIABLE");StoreTransition(savedNameArray[0]+12,foreign);
 AssertTransition(!EA_AllowSavedHelmetKey(reinterpret_cast<std::uintptr_t>(loadKey.data()),0));
 expectedNativeCreate=0;const auto writesBeforeForeign=savedWrites;
 SavedHelmetInt(reinterpret_cast<void*>(loadSource+0x2e0),0,loadKey.data(),0,0,1);
 AssertTransition(savedWrites==writesBeforeForeign+1);expectedNativeCreate=1;savedNameNode=originalNameNode;
 saveHelmetChoices=false;AssertTransition(!EA_AllowSavedHelmetKey(reinterpret_cast<std::uintptr_t>(loadKey.data()),0));
 saveHelmetChoices=false;savedChoices.clear();hiddenHelmetActors.clear();
 // A bounded synthetic MMH exposes the actual 6005 mesh-resource string field.
 std::vector<std::byte> mmhFixture(96);std::memcpy(mmhFixture.data(),"GFF V4.0PC  MMH ",16);
 const auto putFixture=[&](unsigned at,unsigned value){std::memcpy(mmhFixture.data()+at,&value,4);};
 putFixture(20,1);putFixture(24,56);putFixture(32,1);putFixture(36,44);putFixture(40,4);putFixture(44,6005);putFixture(48,14);putFixture(52,0);putFixture(56,4);putFixture(60,11);
 std::memcpy(mmhFixture.data()+64,L"w_test.msh",22);AssertTransition(WeaponMeshName(mmhFixture)=="w_test.msh");
 putFixture(48,0);AssertTransition(!WeaponMeshName(mmhFixture));
 // Real on-disk MMH files, rather than only a synthetic GFF fixture.
 AssertTransition(argc==2);
 for(const auto& file:{"w_stf_st01.mmh","w_stf_st01a_0.mmh"}){
  std::ifstream input(std::filesystem::path(argv[1])/file,std::ios::binary|std::ios::ate);AssertTransition(bool(input));
  const auto length=input.tellg();AssertTransition(length>0 && length<1024*1024);std::vector<std::byte> bytes(static_cast<std::size_t>(length));input.seekg(0);input.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
  const auto expected=std::filesystem::path(file).replace_extension(".msh").string();AssertTransition(WeaponMeshName(bytes)==expected);
 }
 // Read-only weapon observers must forward all stack arguments and preserve full EAX.
 std::array<std::byte,0x260> wield{};std::array<std::byte,0x200> weaponModel{},weaponNode{},weaponAppearance{};
 std::array<std::uintptr_t,2> weaponRef{0,reinterpret_cast<std::uintptr_t>(weaponModel.data())+8};
 expectedWeaponWield=reinterpret_cast<std::uintptr_t>(wield.data());StoreTransition(expectedWeaponWield+0x230,reinterpret_cast<std::uintptr_t>(weaponRef.data()));
 const auto weaponName=dao::core::WStringRep::Borrow(L"w_stf_st01a_0.mmh");expectedWeaponName=reinterpret_cast<std::uintptr_t>(&weaponName);expectedWeaponParts=reinterpret_cast<std::uintptr_t>(weaponNode.data());
 expectedWeaponAppearance=reinterpret_cast<std::uintptr_t>(weaponAppearance.data());StoreTransition(expectedWeaponAppearance+0x58,std::uint16_t{15});StoreTransition(expectedWeaponAppearance+0x50,std::uint8_t{1});
 EA_WeaponLoadOriginal=&WeaponLoadMock;EA_WeaponAttachOriginal=&WeaponAttachMock;
 const auto unchangedWield=wield;const auto unchangedAppearance=weaponAppearance;
 for(unsigned tick=0;tick<100;++tick){
  AssertTransition(WeaponLoadProbe(wield.data(),&weaponName,weaponNode.data(),3,123,1)==0x13579b01);
  AssertTransition(WeaponAttachProbe(wield.data(),weaponAppearance.data(),3,2)==0x2468ac01);
 }
 AssertTransition(wield==unchangedWield && weaponAppearance==unchangedAppearance && weaponBindings.at(weaponRef[1]).attached);
 AssertTransition(weaponBindings.at(weaponRef[1]).row==15 && weaponBindings.at(weaponRef[1]).gender==1);
 // Native callers pass BYTE slots with unspecified high stack bytes.
 EA_WeaponLoaded(expectedWeaponWield,expectedWeaponName,0x0069f103u,1);
 EA_WeaponAttached(expectedWeaponWield,expectedWeaponAppearance,0x3b390004u,1);
 AssertTransition(weaponBindings.at(weaponRef[1]).slot==3);
 EA_WeaponAttached(expectedWeaponWield,expectedWeaponAppearance,0x3b390003u,1);
 AssertTransition(weaponBindings.at(weaponRef[1]).attached);
 AssertTransition(weaponBindings.at(weaponRef[1]).row==15);
 // Exercise the actual per-instance transform gate used by the Node callback.
 std::array<std::byte,0x200> definition{};const auto rootName=dao::core::WStringRep::Borrow(L"GOB");
 StoreTransition(reinterpret_cast<std::uintptr_t>(definition.data())+0xc0,rootName);
 StoreTransition(reinterpret_cast<std::uintptr_t>(weaponNode.data())+0x154,weaponRef[1]-8);
 StoreTransition(reinterpret_cast<std::uintptr_t>(weaponNode.data())+0x174,reinterpret_cast<std::uintptr_t>(definition.data()));
 std::array<std::uintptr_t,2> sourceRef{0,0x12345678};StoreTransition(expectedWeaponWield+0x18,reinterpret_cast<std::uintptr_t>(sourceRef.data()));
 auto& weaponBinding=weaponBindings.at(weaponRef[1]);weaponBinding.sourceModel=sourceRef[1];weaponBinding.factor=.85;weaponBinding.ready=true;weaponBinding.attached=true;
 const heels::Matrix wi{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};heels::Frame ws{};
 weaponConfig.tableEnabled=true;
 for(unsigned repeat=0;repeat<100;++repeat){AssertTransition(WeaponScaleNodeFrame(weaponNode.data(),wi.data(),wi.data(),wi.data(),ws));AssertTransition(std::abs(ws.parent[0]-.85f)<.00001f);}
 weaponConfig.tableEnabled=false;weaponConfig.creatureEnabled=false;AssertTransition(!WeaponScaleNodeFrame(weaponNode.data(),wi.data(),wi.data(),wi.data(),ws));
 weaponConfig.creatureEnabled=true;AssertTransition(WeaponScaleNodeFrame(weaponNode.data(),wi.data(),wi.data(),wi.data(),ws));
 sourceRef[1]=0xabcdef;AssertTransition(!WeaponScaleNodeFrame(weaponNode.data(),wi.data(),wi.data(),wi.data(),ws));sourceRef[1]=weaponBinding.sourceModel;
 const auto savedModel=weaponRef[1];weaponRef[1]=0;AssertTransition(!WeaponScaleNodeFrame(weaponNode.data(),wi.data(),wi.data(),wi.data(),ws));weaponRef[1]=savedModel;
 weaponBinding.ready=false;AssertTransition(!WeaponScaleNodeFrame(weaponNode.data(),wi.data(),wi.data(),wi.data(),ws));weaponBinding.ready=true;
 weaponBinding.rootNode=0;const auto childName=dao::core::WStringRep::Borrow(L"child");StoreTransition(reinterpret_cast<std::uintptr_t>(definition.data())+0xc0,childName);
 AssertTransition(!WeaponScaleNodeFrame(weaponNode.data(),wi.data(),wi.data(),wi.data(),ws));
 StoreTransition(reinterpret_cast<std::uintptr_t>(definition.data())+0xc0,rootName);
 nodeOriginal=&WeaponNodeForwardMock;expectedNativeWeaponMatrix=wi.data();forwardedWeaponFactor=.85;depth=1;
 Node(weaponNode.data(),0,wi.data(),wi.data(),wi.data());AssertTransition(depth==1 && weaponNodeForwards==1);
 auto nativeTraversal=wi;nativeTraversal[3]=-217.3f;nativeTraversal[7]=-335.1f;
 Node(weaponNode.data(),0,wi.data(),wi.data(),nativeTraversal.data());AssertTransition(depth==1 && weaponNodeForwards==2);
 weaponConfig={};forwardedWeaponFactor=1;Node(weaponNode.data(),0,wi.data(),wi.data(),wi.data());AssertTransition(depth==1 && weaponNodeForwards==3);depth=0;
 AssertTransition(wi[0]==1 && weaponRef[1]==savedModel);
 // Cold first visual update resolves before the original renderer callback.
 std::ifstream coldFile(std::filesystem::path(argv[1])/"w_stf_st01a_0.mmh",std::ios::binary|std::ios::ate);AssertTransition(bool(coldFile));
 coldWeaponMMH.resize(static_cast<std::size_t>(coldFile.tellg()));coldFile.seekg(0);coldFile.read(reinterpret_cast<char*>(coldWeaponMMH.data()),coldWeaponMMH.size());
 weaponResourceRead=&ColdWeaponReadMock;weaponGdaString=&ColdWeaponGdaMock;
 weaponTable.meshes["w_stf_st01a_0.msh"]={true,.85,daoaddr::ToHex(daoaddr::Sha256(coldWeaponMSH)),{}};
 weaponConfig.tableEnabled=true;weaponGameThread.store(GetCurrentThreadId());weaponBinding.ready=false;weaponBinding.reported=false;
 weaponResolving=true;AssertTransition(!WeaponScaleNodeFrame(weaponNode.data(),wi.data(),wi.data(),wi.data(),ws));AssertTransition(coldWeaponReads==0);weaponResolving=false;
 forwardedWeaponFactor=.85;depth=1;Node(weaponNode.data(),0,wi.data(),wi.data(),nativeTraversal.data());depth=0;
 AssertTransition(coldWeaponReads==2 && weaponBinding.ready && weaponBinding.factor==.85);
 AssertTransition(weaponPlanCache.at(WeaponKey(weaponBinding))==.85);
 // Recreated instance has no ready factor. Pre-attach bridge must restore
 // the verified factor before the native attach callback can traverse it.
 weaponBinding.ready=false;weaponBinding.factor=1;checkPreparedWeapon=true;
 AssertTransition(WeaponAttachProbe(wield.data(),weaponAppearance.data(),3,2)==0x2468ac01);checkPreparedWeapon=false;
 AssertTransition(weaponBinding.ready && weaponBinding.factor==.85 && coldWeaponReads==2);
 auto other=weaponBinding;other.gender=2;AssertTransition(!weaponPlanCache.contains(WeaponKey(other)));
 EA_WeaponAttached(expectedWeaponWield,expectedWeaponAppearance,3,0);AssertTransition(!weaponBinding.attached && !weaponBinding.ready);
 weaponGameThread=0;weaponResourceRead=&dao::resources::Read;weaponGdaString=&dao::gda::GetString;weaponPlanCache.clear();weaponConfig={};weaponTable={};
 AssertTransition(!WeaponProbeModel(expectedWeaponWield,0));weaponBindings.clear();
 stageState={};
 std::cout<<"PASS: "<<checks<<" production transition checks; immediate dialogue entry/exit rebinding, independent actors, initialization pose and stale object rejection\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}


