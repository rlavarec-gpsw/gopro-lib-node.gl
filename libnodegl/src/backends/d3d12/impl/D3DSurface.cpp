#include <stdafx.h>
#include "D3DSurface.h"

namespace ngli
{

D3DSurface::D3DSurface(uint32_t w, uint32_t h, bool offscreen)
	:mW(w)
	,mH(h)
	,mOffscreen(offscreen)
{
}

D3DSurface* D3DSurface::createFromWindowHandle(uint32_t w, uint32_t h, void* handle)
{
	D3DSurface* d3dSurface = new D3DSurface();
	d3dSurface->mW = w;
	d3dSurface->mH = h;
	d3dSurface->mHwnd = (HWND)handle;
	return d3dSurface;
}



}