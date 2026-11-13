// Test-only DLL implementing the versioned query contract, never shipped as a plugin.
#include <cstdint>
static std::int32_t value=0;
extern "C" __declspec(dllexport) void __cdecl TestSetVariation(std::int32_t row){value=row;}
extern "C" __declspec(dllexport) bool __cdecl IV_GetAppearanceVariationV1(
    const void* appearance,std::uint32_t slot,std::int32_t* output) noexcept {
    if(!appearance || slot!=0 || !output || value<=0)return false;
    *output=value;return true;
}
