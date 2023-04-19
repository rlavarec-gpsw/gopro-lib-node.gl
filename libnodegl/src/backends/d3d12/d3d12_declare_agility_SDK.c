
// Need to declare this to use Agility SDK inside the main exe using node.gl!
// https://devblogs.microsoft.com/directx/directx12agility/

__declspec(dllexport) extern const unsigned int D3D12SDKVersion = 610; // SDK 1.610.0

__declspec(dllexport) extern const char* D3D12SDKPath = u8".\\D3D12\\";
