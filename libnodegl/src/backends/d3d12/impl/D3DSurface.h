#pragma once

namespace ngli
{

class D3DSurface
{
public:

    D3DSurface() {}

    /// @brief Construct the surface object
    /// @param w 
    /// @param h 
    /// @param offscreen true if offscreen
    D3DSurface(uint32_t w, uint32_t h, bool offscreen = false);

    /// @brief Create D3DSurface from HWND handle
    /// @param w 
    /// @param h 
    /// @param handle 
    /// @return a surface from a HWND handle 
    static D3DSurface* createFromWindowHandle(uint32_t w, uint32_t h, void* handle);

public:
    // The surface width
    uint32_t mW = 0;

    // The surface height
    uint32_t mH = 0;

    // If true, the surface is offscreen
    bool mOffscreen = false;

    // Window handle
	HWND mHwnd;
};

}