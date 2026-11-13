#include "dao/Plugin.h"
#include <Windows.h>
#include <iostream>
int wmain(int argc,wchar_t**argv){
 if(argc!=2)return 1;auto module=LoadLibraryW(argv[1]);if(!module)return 2;
 auto load=reinterpret_cast<dao::LoadFn>(GetProcAddress(module,"DAOLib_Load"));if(!load)return 3;
 if(load(nullptr))return 4;dao::LoaderInfo wrong{};wrong.apiVersion=999;wrong.structSize=sizeof(wrong);
 if(load(&wrong))return 5;FreeLibrary(module);
 std::cout<<"PASS: real x86 Equipment Appearance DLL loads and rejects incompatible loader ABI\n";
}
