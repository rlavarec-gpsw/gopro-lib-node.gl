
#include <vector>
#include <map>
#include <set>
#include <mutex>
#include <functional>
#include <optional>
#include <cassert>
#include <string>
#include <cstring>
#include <ctime>
#include <fstream>
#include <filesystem>
#include <regex>

#define NOMINMAX
#include <windows.h>

#include <d3dx12/d3dx12.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <system_error>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define DEBUG_D3D12_ACTIVATED ((DEBUG_D3D12 | DEBUG_D3D12_GPU_VALIDATION | DEBUG_D3D12_TRACE) > 0?true:false)

#include <drawutils.h>

#include <backends/common/StringUtil.h>

#include <backends/d3d12/impl/D3DUtils.h>
#include <backends/d3d12/impl/D3DGraphicsCore.h>

