#include <array>
#include "GuiCodeGuards.h"
#include <iostream>
namespace {const daoaddr::Library* library=nullptr;unsigned checks=0;void Check(bool value){++checks;if(!value)throw std::runtime_error("GUI disk guard check failed "+std::to_string(checks));}
std::vector<unsigned char> ReadFile(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary|std::ios::ate);auto size=f.tellg();Check(size>0);std::vector<unsigned char>b(static_cast<std::size_t>(size));f.seekg(0);f.read(reinterpret_cast<char*>(b.data()),size);return b;}
void WriteFile(const std::filesystem::path& p,const std::vector<unsigned char>& b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());}
}
bool VerifyAll(const std::filesystem::path& p){return VerifyGuiDisk(p) && VerifyWeaponProbeDisk(p);}
namespace dao::Module {const daoaddr::Library* Library(){return library;}}
int wmain(int argc,wchar_t**argv){try{
 Check(argc==5);auto lib=daoaddr::Library::Load(argv[3]);Check(lib.has_value());library=&*lib;Check(VerifyAll(argv[1]));Check(VerifyAll(argv[2]));
 auto b=ReadFile(argv[1]);unsigned pe=0;std::memcpy(&pe,b.data()+60,4);auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(b.data()+pe);const auto sections=IMAGE_FIRST_SECTION(nt);
 for(auto pair:std::array{std::pair{0x2a1240u,80u},std::pair{0x2a1350u,112u},std::pair{0x3662a0u,720u},std::pair{0x12eb0u,592u},std::pair{0x1113a0u,128u},std::pair{0x11f5f0u,80u},std::pair{0x11f640u,96u},std::pair{0x2c1400u,736u},std::pair{0xe8750u,368u},std::pair{0x303a00u,496u},std::pair{0x303bf0u,48u},std::pair{0x303db0u,64u},std::pair{0x256120u,368u}}){
  unsigned offset=0;for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i)if(sections[i].VirtualAddress<=pair.first && pair.first<sections[i].VirtualAddress+sections[i].SizeOfRawData)offset=sections[i].PointerToRawData+pair.first-sections[i].VirtualAddress;Check(offset>0);
  for(auto pos:std::array{0u,pair.second/2,pair.second-1}){b[offset+pos]^=1;WriteFile(argv[4],b);Check(!VerifyAll(argv[4]));b[offset+pos]^=1;}
 }
 library=nullptr;Check(!VerifyAll(argv[1]));library=&*lib;
 WriteFile(argv[4],{});Check(!VerifyAll(argv[4]));b.assign(64,0);WriteFile(argv[4],b);Check(!VerifyAll(argv[4]));
 std::cout<<"PASS: "<<checks<<" GUI disk guard checks; GOG/Steam accepted, altered blocks/truncated headers and unknown profile rejected\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

