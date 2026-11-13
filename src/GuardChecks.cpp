
#include "dao/Module.h"
#include "dao/SafeRead.h"

#include <Windows.h>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#include <source_location>
#include "Guards.h"
namespace {std::byte* image=nullptr;const daoaddr::Library* library=nullptr;unsigned count=0;
void Check(bool b,const std::source_location loc=std::source_location::current()){++count;if(!b)throw std::runtime_error("portable guard check failed at line "+std::to_string(loc.line()));}
std::vector<std::byte> Read(const std::filesystem::path&p){std::ifstream in(p,std::ios::binary|std::ios::ate);Check(in.good());auto n=in.tellg();in.seekg(0);std::vector<std::byte>b(static_cast<std::size_t>(n));in.read(reinterpret_cast<char*>(b.data()),n);return b;}
}

namespace dao {
bool TryCopy(void*out,std::uintptr_t address,std::size_t size) noexcept{SIZE_T n=0;return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),out,size,&n)&&n==size;}
namespace Module {
const daoaddr::Library* Library(){return library;}
std::uintptr_t Base(){return reinterpret_cast<std::uintptr_t>(image);}
std::uintptr_t Address(AddressId id){const auto r=library->Rva(id);return r?Base()+*r:0;}
}}
int wmain(int argc,wchar_t**argv){try{
 Check(argc==3);const auto folder=std::filesystem::path(argv[2]);
 const std::array names{L"daoaddresses-4eb19d53-009ec0a2-004e3024-00400000.bin",L"daoaddresses-4eb19d53-00a49000-009ed2ee-00400000.bin",L"daoaddresses-52164c37-0096b000-00001000-00400000.bin"};
 auto gog=daoaddr::Library::Load(folder/names[0]);library=&*gog;
 auto file=Read(argv[1]);auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);
 image=static_cast<std::byte*>(VirtualAlloc(reinterpret_cast<void*>(nt->OptionalHeader.ImageBase),nt->OptionalHeader.SizeOfImage,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 if(!image)image=static_cast<std::byte*>(VirtualAlloc(nullptr,nt->OptionalHeader.SizeOfImage,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));Check(image!=nullptr);
 std::memcpy(image,file.data(),nt->OptionalHeader.SizeOfHeaders);const auto sections=IMAGE_FIRST_SECTION(nt);
 for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i)std::memcpy(image+sections[i].VirtualAddress,file.data()+sections[i].PointerToRawData,sections[i].SizeOfRawData);
 const auto delta=static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(image)-nt->OptionalHeader.ImageBase);
 const auto reloc=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
 for(std::size_t pos=reloc.VirtualAddress;pos<reloc.VirtualAddress+reloc.Size;){auto*block=reinterpret_cast<IMAGE_BASE_RELOCATION*>(image+pos);if(!block->SizeOfBlock)break;
 const auto*words=reinterpret_cast<const WORD*>(block+1);for(unsigned i=0;i<(block->SizeOfBlock-8)/2;++i)if(words[i]>>12==IMAGE_REL_BASED_HIGHLOW){auto*ptr=reinterpret_cast<std::uint32_t*>(image+block->VirtualAddress+(words[i]&4095));*ptr+=delta;}pos+=block->SizeOfBlock;}

 Check(Verify());
 // Every guarded byte is independently corrupted; the guard must reject it.
 for(auto pair:std::array{std::pair{14160u,112u},std::pair{33448u,160u},std::pair{8348u,688u},std::pair{7596u,16u},std::pair{10504u,32u},std::pair{12246u,16u},std::pair{16002u,208u},std::pair{16007u,48u},std::pair{13022u,400u},std::pair{15988u,96u},std::pair{13376u,368u},std::pair{19468u,128u},std::pair{18162u,16u},std::pair{21362u,64u},std::pair{21370u,48u},std::pair{19464u,32u},std::pair{12954u,48u},std::pair{32471u,64u},std::pair{14970u,784u},std::pair{10509u,80u},std::pair{13161u,160u},std::pair{13897u,1152u},std::pair{13891u,864u},std::pair{15033u,416u},std::pair{15035u,240u},std::pair{14446u,768u},std::pair{13951u,784u},std::pair{2724u,128u},std::pair{13932u,208u},std::pair{2723u,224u},std::pair{2729u,64u},std::pair{14447u,224u},std::pair{8326u,48u},std::pair{8314u,32u},std::pair{8310u,16u},std::pair{8733u,16u},std::pair{9643u,16u},std::pair{20120u,928u},std::pair{18290u,96u},std::pair{20210u,400u},std::pair{20242u,240u},std::pair{18327u,672u},std::pair{20296u,1504u},std::pair{20055u,176u},std::pair{17835u,640u},std::pair{17767u,16u},std::pair{17750u,32u},std::pair{17752u,32u},std::pair{21050u,1664u},std::pair{20638u,144u},std::pair{20724u,64u},std::pair{17790u,192u},std::pair{14965u,1328u}}){
  const auto rva=gog->Rva(daoaddr::Id{pair.first});Check(rva.has_value());
  for(unsigned n=0;n<pair.second;++n){auto& v=image[*rva+n];v^=std::byte{1};Check(!Verify());v^=std::byte{1};}
 }
 Check(Verify());VirtualFree(image,0,MEM_RELEASE);
 std::cout<<"PASS: "<<count<<" guard checks; real image mapped as data only, every guarded byte corruption rejected\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
