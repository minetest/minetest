// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#define _IRR_DONT_DO_MEMORY_DEBUGGING_HERE
#include "CD3D9Driver.h"

#ifdef _IRR_COMPILE_WITH_DIRECT3D_9_

#include "os.h"
#include "S3DVertex.h"
#include "CD3D9Texture.h"
#include "CD3D9MaterialRenderer.h"
#include "CD3D9ShaderMaterialRenderer.h"
#include "CD3D9NormalMapRenderer.h"
#include "CD3D9ParallaxMapRenderer.h"
#include "CD3D9HLSLMaterialRenderer.h"
#include "CD3D9CgMaterialRenderer.h"
#include "SIrrCreationParameters.h"

namespace irr
{
namespace video
{

namespace
{
	inline DWORD F2DW( FLOAT f ) { return *((DWORD*)&f); }
}

//! constructor
CD3D9Driver::CD3D9Driver(const SIrrlichtCreationParameters& params, io::IFileSystem* io)
	: CNullDriver(io, params.WindowSize), CurrentRenderMode(ERM_NONE),
	ResetRenderStates(true), Transformation3DChanged(false),
	D3DLibrary(0), pID3D(0), pID3DDevice(0), PrevRenderTarget(0),
	WindowId(0), SceneSourceRect(0),
	LastVertexType((video::E_VERTEX_TYPE)-1), VendorID(0),
	MaxTextureUnits(0), MaxUserClipPlanes(0), MaxMRTs(1), NumSetMRTs(1),
	MaxLightDistance(0.f), LastSetLight(-1),
	ColorFormat(ECF_A8R8G8B8), DeviceLost(false),
	DriverWasReset(true), OcclusionQuerySupport(false),
	AlphaToCoverageSupport(false), Params(params)
{
	#ifdef _DEBUG
	setDebugName("CD3D9Driver");
	#endif

	printVersion();

	for (u32 i=0; i<MATERIAL_MAX_TEXTURES; ++i)
	{
		CurrentTexture[i] = 0;
		LastTextureMipMapsAvailable[i] = false;
	}
	MaxLightDistance = sqrtf(FLT_MAX);
	// create sphere map matrix

	SphereMapMatrixD3D9._11 = 0.5f; SphereMapMatrixD3D9._12 = 0.0f;
	SphereMapMatrixD3D9._13 = 0.0f; SphereMapMatrixD3D9._14 = 0.0f;
	SphereMapMatrixD3D9._21 = 0.0f; SphereMapMatrixD3D9._22 =-0.5f;
	SphereMapMatrixD3D9._23 = 0.0f; SphereMapMatrixD3D9._24 = 0.0f;
	SphereMapMatrixD3D9._31 = 0.0f; SphereMapMatrixD3D9._32 = 0.0f;
	SphereMapMatrixD3D9._33 = 1.0f; SphereMapMatrixD3D9._34 = 0.0f;
	SphereMapMatrixD3D9._41 = 0.5f; SphereMapMatrixD3D9._42 = 0.5f;
	SphereMapMatrixD3D9._43 = 0.0f; SphereMapMatrixD3D9._44 = 1.0f;

	core::matrix4 mat;
	UnitMatrixD3D9 = *(D3DMATRIX*)((void*)mat.pointer());

	#ifdef _IRR_COMPILE_WITH_CG_
	CgContext = 0;
	#endif

	// init direct 3d is done in the factory function
}


//! destructor
CD3D9Driver::~CD3D9Driver()
{
	deleteMaterialRenders();
	deleteAllTextures();
	removeAllOcclusionQueries();
	removeAllHardwareBuffers();
	for (u32 i=0; i<DepthBuffers.size(); ++i)
	{
		DepthBuffers[i]->drop();
	}
	DepthBuffers.clear();

	// drop d3d9

	if (pID3DDevice)
		pID3DDevice->Release();

	if (pID3D)
		pID3D->Release();

	#ifdef _IRR_COMPILE_WITH_CG_
	cgD3D9SetDevice(0);

	if(CgContext)
	{
		cgDestroyContext(CgContext);
	}
	#endif
}


void CD3D9Driver::createMaterialRenderers()
{
	// create D3D9 material renderers

	addAndDropMaterialRenderer(new CD3D9MaterialRenderer_SOLID(pID3DDevice, this));
	addAndDropMaterialRenderer(new CD3D9MaterialRenderer_SOLID_2_LAYER(pID3DDevice, this));

	// add the same renderer for all lightmap types

	CD3D9MaterialRenderer_LIGHTMAP* lmr = new CD3D9MaterialRenderer_LIGHTMAP(pID3DDevice, this);
	addMaterialRenderer(lmr); // for EMT_LIGHTMAP:
	addMaterialRenderer(lmr); // for EMT_LIGHTMAP_ADD:
	addMaterialRenderer(lmr); // for EMT_LIGHTMAP_M2:
	addMaterialRenderer(lmr); // for EMT_LIGHTMAP_M4:
	addMaterialRenderer(lmr); // for EMT_LIGHTMAP_LIGHTING:
	addMaterialRenderer(lmr); // for EMT_LIGHTMAP_LIGHTING_M2:
	addMaterialRenderer(lmr); // for EMT_LIGHTMAP_LIGHTING_M4:
	lmr->drop();

	// add remaining fixed function pipeline material renderers

	addAndDropMaterialRenderer(new CD3D9MaterialRenderer_DETAIL_MAP(pID3DDevice, this));
	addAndDropMaterialRenderer(new CD3D9MaterialRenderer_SPHERE_MAP(pID3DDevice, this));
	addAndDropMaterialRenderer(new CD3D9MaterialRenderer_REFLECTION_2_LAYER(pID3DDevice, this));
	addAndDropMaterialRenderer(new CD3D9MaterialRenderer_TRANSPARENT_ADD_COLOR(pID3DDevice, this));
	addAndDropMaterialRenderer(new CD3D9MaterialRenderer_TRANSPARENT_ALPHA_CHANNEL(pID3DDevice, this));
	addAndDropMaterialRenderer(new CD3D9MaterialRenderer_TRANSPARENT_ALPHA_CHANNEL_REF(pID3DDevice, this));
	addAndDropMaterialRenderer(new CD3D9MaterialRenderer_TRANSPARENT_VERTEX_ALPHA(pID3DDevice, this));
	addAndDropMaterialRenderer(new CD3D9MaterialRenderer_TRANSPARENT_REFLECTION_2_LAYER(pID3DDevice, this));

	// add normal map renderers

	s32 tmp = 0;
	video::IMaterialRenderer* renderer = 0;

	renderer = new CD3D9NormalMapRenderer(pID3DDevice, this, tmp,
		MaterialRenderers[EMT_SOLID].Renderer);
	renderer->drop();

	renderer = new CD3D9NormalMapRenderer(pID3DDevice, this, tmp,
		MaterialRenderers[EMT_TRANSPARENT_ADD_COLOR].Renderer);
	renderer->drop();

	renderer = new CD3D9NormalMapRenderer(pID3DDevice, this, tmp,
		MaterialRenderers[EMT_TRANSPARENT_VERTEX_ALPHA].Renderer);
	renderer->drop();

	// add parallax map renderers

	renderer = new CD3D9ParallaxMapRenderer(pID3DDevice, this, tmp,
		MaterialRenderers[EMT_SOLID].Renderer);
	renderer->drop();

	renderer = new CD3D9ParallaxMapRenderer(pID3DDevice, this, tmp,
		MaterialRenderers[EMT_TRANSPARENT_ADD_COLOR].Renderer);
	renderer->drop();

	renderer = new CD3D9ParallaxMapRenderer(pID3DDevice, this, tmp,
		MaterialRenderers[EMT_TRANSPARENT_VERTEX_ALPHA].Renderer);
	renderer->drop();

	// add basic 1 texture blending
	addAndDropMaterialRenderer(new CD3D9MaterialRenderer_ONETEXTURE_BLEND(pID3DDevice, this));
}


//! initialises the Direct3D API
bool CD3D9Driver::initDriver(HWND hwnd, bool pureSoftware)
{
	if (!pID3D)
	{
		D3DLibrary = LoadLibrary( __TEXT("d3d9.dll") );

		if (!D3DLibrary)
		{
			os::Printer::log("Error, could not load d3d9.dll.", ELL_ERROR);
			return false;
		}

		typedef IDirect3D9 * (__stdcall *D3DCREATETYPE)(UINT);
		D3DCREATETYPE d3dCreate = (D3DCREATETYPE) GetProcAddress(D3DLibrary, "Direct3DCreate9");

		if (!d3dCreate)
		{
			os::Printer::log("Error, could not get proc adress of Direct3DCreate9.", ELL_ERROR);
			return false;
		}

		//just like pID3D = Direct3DCreate9(D3D_SDK_VERSION);
		pID3D = (*d3dCreate)(D3D_SDK_VERSION);

		if (!pID3D)
		{
			os::Printer::log("Error initializing D3D.", ELL_ERROR);
			return false;
		}
	}

	// print device information
	D3DADAPTER_IDENTIFIER9 dai;
	if (!FAILED(pID3D->GetAdapterIdentifier(Params.DisplayAdapter, 0, &dai)))
	{
		char tmp[512];

		s32 Product = HIWORD(dai.DriverVersion.HighPart);
		s32 Version = LOWORD(dai.DriverVersion.HighPart);
		s32 SubVersion = HIWORD(dai.DriverVersion.LowPart);
		s32 Build = LOWORD(dai.DriverVersion.LowPart);

		sprintf(tmp, "%s %s %d.%d.%d.%d", dai.Description, dai.Driver, Product, Version,
			SubVersion, Build);
		os::Printer::log(tmp, ELL_INFORMATION);

		// Assign vendor name based on vendor id.
		VendorID= static_cast<u16>(dai.VendorId);
		switch(dai.VendorId)
		{
			case 0x1002 : VendorName = "ATI Technologies Inc."; break;
			case 0x10DE : VendorName = "NVIDIA Corporation"; break;
			case 0x102B : VendorName = "Matrox Electronic Systems Ltd."; break;
			case 0x121A : VendorName = "3dfx Interactive Inc"; break;
			case 0x5333 : VendorName = "S3 Graphics Co., Ltd."; break;
			case 0x8086 : VendorName = "Intel Corporation"; break;
			default: VendorName = "Unknown VendorId: ";VendorName += (u32)dai.VendorId; break;
		}
	}

	D3DDISPLAYMODE d3ddm;
	if (FAILED(pID3D->GetAdapterDisplayMode(Params.DisplayAdapter, &d3ddm)))
	{
		os::Printer::log("Error: Could not get Adapter Display mode.", ELL_ERROR);
		return false;
	}

	ZeroMemory(&present, sizeof(present));

	present.BackBufferCount = 1;
	present.EnableAutoDepthStencil = TRUE;
	if (Params.Vsync)
		present.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	else
		present.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	if (Params.Fullscreen)
	{
		present.BackBufferWidth = Params.WindowSize.Width;
		present.BackBufferHeight = Params.WindowSize.Height;
		// request 32bit mode if user specified 32 bit, added by Thomas Stuefe
		if (Params.Bits == 32)
			present.BackBufferFormat = D3DFMT_X8R8G8B8;
		else
			present.BackBufferFormat = D3DFMT_R5G6B5;
		present.SwapEffect	= D3DSWAPEFFECT_FLIP;
		present.Windowed	= FALSE;
		present.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	}
	else
	{
		present.BackBufferFormat	= d3ddm.Format;
		present.SwapEffect		= D3DSWAPEFFECT_DISCARD;
		present.Windowed		= TRUE;
	}

	UINT adapter = Params.DisplayAdapter;
	D3DDEVTYPE devtype = D3DDEVTYPE_HAL;
	#ifndef _IRR_D3D_NO_SHADER_DEBUGGING
	devtype = D3DDEVTYPE_REF;
	#elif defined(_IRR_USE_NVIDIA_PERFHUD_)
	for (UINT adapter_i = 0; adapter_i < pID3D->GetAdapterCount(); ++adapter_i)
	{
		D3DADAPTER_IDENTIFIER9 identifier;
		pID3D->GetAdapterIdentifier(adapter_i,0,&identifier);
		if (strstr(identifier.Description,"PerfHUD") != 0)
		{
			adapter = adapter_i;
			devtype = D3DDEVTYPE_REF;
			break;
		}
	}
	#endif

	// enable anti alias if possible and desired
	if (Params.AntiAlias > 0)
	{
		if (Params.AntiAlias > 32)
			Params.AntiAlias = 32;

		DWORD qualityLevels = 0;

		while(Params.AntiAlias > 0)
		{
			if(SUCCEEDED(pID3D->CheckDeviceMultiSampleType(adapter,
				devtype, present.BackBufferFormat, !Params.Fullscreen,
				(D3DMULTISAMPLE_TYPE)Params.AntiAlias, &qualityLevels)))
			{
				present.MultiSampleType	= (D3DMULTISAMPLE_TYPE)Params.AntiAlias;
				present.MultiSampleQuality = qualityLevels-1;
				present.SwapEffect	 = D3DSWAPEFFECT_DISCARD;
				break;
			}
			--Params.AntiAlias;
		}

		if (Params.AntiAlias==0)
		{
			os::Printer::log("Anti aliasing disabled because hardware/driver lacks necessary caps.", ELL_WARNING);
		}
	}

	// check stencil buffer compatibility
	if (Params.Stencilbuffer)
	{
		present.AutoDepthStencilFormat = D3DFMT_D24S8;
		if(FAILED(pID3D->CheckDeviceFormat(adapter, devtype,
			present.BackBufferFormat, D3DUSAGE_DEPTHSTENCIL,
			D3DRTYPE_SURFACE, present.AutoDepthStencilFormat)))
		{
			present.AutoDepthStencilFormat = D3DFMT_D24X4S4;
			if(FAILED(pID3D->CheckDeviceFormat(adapter, devtype,
				present.BackBufferFormat, D3DUSAGE_DEPTHSTENCIL,
				D3DRTYPE_SURFACE, present.AutoDepthStencilFormat)))
			{
				present.AutoDepthStencilFormat = D3DFMT_D15S1;
				if(FAILED(pID3D->CheckDeviceFormat(adapter, devtype,
					present.BackBufferFormat, D3DUSAGE_DEPTHSTENCIL,
					D3DRTYPE_SURFACE, present.AutoDepthStencilFormat)))
				{
					os::Printer::log("Device does not support stencilbuffer, disabling stencil buffer.", ELL_WARNING);
					Params.Stencilbuffer = false;
				}
			}
		}
		else
		if(FAILED(pID3D->CheckDepthStencilMatch(adapter, devtype,
			present.BackBufferFormat, present.BackBufferFormat, present.AutoDepthStencilFormat)))
		{
			os::Printer::log("Depth-stencil format is not compatible with display format, disabling stencil buffer.", ELL_WARNING);
			Params.Stencilbuffer = false;
		}
	}
	// do not use else here to cope with flag change in previous block
	if (!Params.Stencilbuffer)
	{
		present.AutoDepthStencilFormat = D3DFMT_D32;
		if(FAILED(pID3D->CheckDeviceFormat(adapter, devtype,
			present.BackBufferFormat, D3DUSAGE_DEPTHSTENCIL,
			D3DRTYPE_SURFACE, present.AutoDepthStencilFormat)))
		{
			present.AutoDepthStencilFormat = D3DFMT_D24X8;
			if(FAILED(pID3D->CheckDeviceFormat(adapter, devtype,
				present.BackBufferFormat, D3DUSAGE_DEPTHSTENCIL,
				D3DRTYPE_SURFACE, present.AutoDepthStencilFormat)))
			{
				present.AutoDepthStencilFormat = D3DFMT_D16;
				if(FAILED(pID3D->CheckDeviceFormat(adapter, devtype,
					present.BackBufferFormat, D3DUSAGE_DEPTHSTENCIL,
					D3DRTYPE_SURFACE, present.AutoDepthStencilFormat)))
				{
					os::Printer::log("Device does not support required depth buffer.", ELL_WARNING);
					return false;
				}
			}
		}
	}

	// create device

	DWORD fpuPrecision = Params.HighPrecisionFPU ? D3DCREATE_FPU_PRESERVE : 0;
	DWORD multithreaded = Params.DriverMultithreaded ? D3DCREATE_MULTITHREADED : 0;
	if (pureSoftware)
	{
		if (FAILED(pID3D->CreateDevice(Params.DisplayAdapter, D3DDEVTYPE_REF, hwnd,
				fpuPrecision | D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present, &pID3DDevice)))
			os::Printer::log("Was not able to create Direct3D9 software device.", ELL_ERROR);
	}
	else
	{
		HRESULT hr = pID3D->CreateDevice(adapter, devtype, hwnd,
				fpuPrecision | multithreaded | D3DCREATE_HARDWARE_VERTEXPROCESSING, &present, &pID3DDevice);

		if(FAILED(hr))
			hr = pID3D->CreateDevice(adapter, devtype, hwnd,
					fpuPrecision | multithreaded | D3DCREATE_MIXED_VERTEXPROCESSING , &present, &pID3DDevice);

		if(FAILED(hr))
			hr = pID3D->CreateDevice(adapter, devtype, hwnd,
					fpuPrecision | multithreaded | D3DCREATE_SOFTWARE_VERTEXPROCESSING, &present, &pID3DDevice);

		if (FAILED(hr))
			os::Printer::log("Was not able to create Direct3D9 device.", ELL_ERROR);
	}

	if (!pID3DDevice)
	{
		os::Printer::log("Was not able to create DIRECT3D9 device.", ELL_ERROR);
		return false;
	}

	// get caps
	pID3DDevice->GetDeviceCaps(&Caps);

	os::Printer::log("Currently available Video Memory (kB)", core::stringc(pID3DDevice->GetAvailableTextureMem()/1024).c_str());

	// disable stencilbuffer if necessary
	if (Params.Stencilbuffer &&
		(!(Caps.StencilCaps & D3DSTENCILCAPS_DECRSAT) ||
		!(Caps.StencilCaps & D3DSTENCILCAPS_INCRSAT) ||
		!(Caps.StencilCaps & D3DSTENCILCAPS_KEEP)))
	{
		os::Printer::log("Device not able to use stencil buffer, disabling stencil buffer.", ELL_WARNING);
		Params.Stencilbuffer = false;
	}

	// set default vertex shader
	setVertexShader(EVT_STANDARD);

	// set fog mode
	setFog(FogColor, FogType, FogStart, FogEnd, FogDensity, PixelFog, RangeFog);

	// set exposed data
	ExposedData.D3D9.D3D9 = pID3D;
	ExposedData.D3D9.D3DDev9 = pID3DDevice;
	ExposedData.D3D9.HWnd = hwnd;

	ResetRenderStates = true;

	// create materials
	createMaterialRenderers();

	MaxTextureUnits = core::min_((u32)Caps.MaxSimultaneousTextures, MATERIAL_MAX_TEXTURES);
	MaxUserClipPlanes = (u32)Caps.MaxUserClipPlanes;
	MaxMRTs = (s32)Caps.NumSimultaneousRTs;
	OcclusionQuerySupport=(pID3DDevice->CreateQuery(D3DQUERYTYPE_OCCLUSION, NULL) == S_OK);

	if (VendorID==0x10DE)//NVidia
		AlphaToCoverageSupport = (pID3D->CheckDeviceFormat(adapter, D3DDEVTYPE_HAL,
				D3DFMT_X8R8G8B8, 0,D3DRTYPE_SURFACE,
				(D3DFORMAT)MAKEFOURCC('A', 'T', 'O', 'C')) == S_OK);
	else if (VendorID==0x1002)//ATI
		AlphaToCoverageSupport = true; // TODO: Check unknown
#if 0
		AlphaToCoverageSupport = (pID3D->CheckDeviceFormat(adapter, D3DDEVTYPE_HAL,
				D3DFMT_X8R8G8B8, 0,D3DRTYPE_SURFACE,
				(D3DFORMAT)MAKEFOURCC('A','2','M','1')) == S_OK);
#endif

	DriverAttributes->setAttribute("MaxTextures", (s32)MaxTextureUnits);
	DriverAttributes->setAttribute("MaxSupportedTextures", (s32)Caps.MaxSimultaneousTextures);
	DriverAttributes->setAttribute("MaxLights", (s32)Caps.MaxActiveLights);
	DriverAttributes->setAttribute("MaxAnisotropy", (s32)Caps.MaxAnisotropy);
	DriverAttributes->setAttribute("MaxUserClipPlanes", (s32)Caps.MaxUserClipPlanes);
	DriverAttributes->setAttribute("MaxMultipleRenderTargets", (s32)Caps.NumSimultaneousRTs);
	DriverAttributes->setAttribute("MaxIndices", (s32)Caps.MaxVertexIndex);
	DriverAttributes->setAttribute("MaxTextureSize", (s32)core::min_(Caps.MaxTextureHeight,Caps.MaxTextureWidth));
	DriverAttributes->setAttribute("MaxTextureLODBias", 16);
	DriverAttributes->setAttribute("Version", 901);
	DriverAttributes->setAttribute("ShaderLanguageVersion", (s32)(((0x00ff00 & Caps.VertexShaderVersion)>>8)*100 + (Caps.VertexShaderVersion&0xff)));
	DriverAttributes->setAttribute("AntiAlias", Params.AntiAlias);

	// set the renderstates
	setRenderStates3DMode();

	// store the screen's depth buffer
	DepthBuffers.push_back(new SDepthSurface());
	if (SUCCEEDED(pID3DDevice->GetDepthStencilSurface(&(DepthBuffers[0]->Surface))))
	{
		D3DSURFACE_DESC desc;
		DepthBuffers[0]->Surface->GetDesc(&desc);
		DepthBuffers[0]->Size.set(desc.Width, desc.Height);
	}
	else
	{
		os::Printer::log("Was not able to get main depth buffer.", ELL_ERROR);
		return false;
	}

	D3DColorFormat = D3DFMT_A8R8G8B8;
	IDirect3DSurface9* bb=0;
	if (SUCCEEDED(pID3DDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb)))
	{
		D3DSURFACE_DESC desc;
		bb->GetDesc(&desc);
		D3DColorFormat = desc.Format;

		if (D3DColorFormat == D3DFMT_X8R8G8B8)
			D3DColorFormat = D3DFMT_A8R8G8B8;

		bb->Release();
	}
	ColorFormat = getColorFormatFromD3DFormat(D3DColorFormat);

	#ifdef _IRR_COMPILE_WITH_CG_
	CgContext = cgCreateContext();
	cgD3D9SetDevice(pID3DDevice);
	#endif

	// so far so good.
	return true;
}


//! applications must call this method before performing any rendering. returns false if failed.
bool CD3D9Driver::beginScene(bool backBuffer, bool zBuffer, SColor color,
		const SExposedVideoData& videoData, core::rect<s32>* sourceRect)
{
	CNullDriver::beginScene(backBuffer, zBuffer, color, videoData, sourceRect);
	WindowId = (HWND)videoData.D3D9.HWnd;
	SceneSourceRect = sourceRect;

	if (!pID3DDevice)
		return false;

	HRESULT hr;
	if (DeviceLost)
	{
		if (FAILED(hr = pID3DDevice->TestCooperativeLevel()))
		{
			if (hr == D3DERR_DEVICELOST)
			{
				Sleep(100);
				hr = pID3DDevice->TestCooperativeLevel();
				if (hr == D3DERR_DEVICELOST)
					return false;
			}

			if ((hr == D3DERR_DEVICENOTRESET) && !reset())
				return false;
		}
	}

	DWORD flags = 0;

	if (backBuffer)
		flags |= D3DCLEAR_TARGET;

	if (zBuffer)
		flags |= D3DCLEAR_ZBUFFER;

	if (Params.Stencilbuffer)
		flags |= D3DCLEAR_STENCIL;

	if (flags)
	{
		hr = pID3DDevice->Clear( 0, NULL, flags, color.color, 1.0, 0);
		if (FAILED(hr))
			os::Printer::log("DIRECT3D9 clear failed.", ELL_WARNING);
	}

	hr = pID3DDevice->BeginScene();
	if (FAILED(hr))
	{
		os::Printer::log("DIRECT3D9 begin scene failed.", ELL_WARNING);
		return false;
	}

	return true;
}


//! applications must call this method after performing any rendering. returns false if failed.
bool CD3D9Driver::endScene()
{
	CNullDriver::endScene();
	DriverWasReset=false;

	HRESULT hr = pID3DDevice->EndScene();
	if (FAILED(hr))
	{
		os::Printer::log("DIRECT3D9 end scene failed.", ELL_WARNING);
		return false;
	}

	RECT* srcRct = 0;
	RECT sourceRectData;
	if ( SceneSourceRect )
	{
		srcRct = &sourceRectData;
		sourceRectData.left = SceneSourceRect->UpperLeftCorner.X;
		sourceRectData.top = SceneSourceRect->UpperLeftCorner.Y;
		sourceRectData.right = SceneSourceRect->LowerRightCorner.X;
		sourceRectData.bottom = SceneSourceRect->LowerRightCorner.Y;
	}

	IDirect3DSwapChain9* swChain;
	hr = pID3DDevice->GetSwapChain(0, &swChain);
	DWORD flags = (Params.HandleSRGB && (Caps.Caps3&D3DCAPS3_LINEAR_TO_SRGB_PRESENTATION))?D3DPRESENT_LINEAR_CONTENT:0;
	hr = swChain->Present(srcRct, NULL, WindowId, NULL, flags);
	swChain->Release();

	if (SUCCEEDED(hr))
		return true;

	if (hr == D3DERR_DEVICELOST)
	{
		DeviceLost = true;
		os::Printer::log("Present failed", "DIRECT3D9 device lost.", ELL_WARNING);
	}
#ifdef D3DERR_DEVICEREMOVED
	else if (hr == D3DERR_DEVICEREMOVED)
	{
		os::Printer::log("Present failed", "Device removed.", ELL_WARNING);
	}
#endif
	else if (hr == D3DERR_INVALIDCALL)
	{
		os::Printer::log("Present failed", "Invalid Call", ELL_WARNING);
	}
	else
		os::Printer::log("DIRECT3D9 present failed.", ELL_WARNING);
	return false;
}


//! queries the features of the driver, returns true if feature is available
bool CD3D9Driver::queryFeature(E_VIDEO_DRIVER_FEATURE feature) const
{
	if (!FeatureEnabled[feature])
		return false;

	switch (feature)
	{
	case EVDF_MULTITEXTURE:
	case EVDF_BILINEAR_FILTER:
		return true;
	case EVDF_RENDER_TO_TARGET:
		return Caps.NumSimultaneousRTs > 0;
	case EVDF_HARDWARE_TL:
		return (Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0;
	case EVDF_MIP_MAP:
		return (Caps.TextureCaps & D3DPTEXTURECAPS_MIPMAP) != 0;
	case EVDF_MIP_MAP_AUTO_UPDATE:
		// always return false because a lot of drivers claim they do
		// this but actually don't do this at all.
		return false; //(Caps.Caps2 & D3DCAPS2_CANAUTOGENMIPMAP) != 0;
	case EVDF_STENCIL_BUFFER:
		return Params.Stencilbuffer && Caps.StencilCaps;
	case EVDF_VERTEX_SHADER_1_1:
		return Caps.VertexShaderVersion >= D3DVS_VERSION(1,1);
	case EVDF_VERTEX_SHADER_2_0:
		return Caps.VertexShaderVersion >= D3DVS_VERSION(2,0);
	case EVDF_VERTEX_SHADER_3_0:
		return Caps.VertexShaderVersion >= D3DVS_VERSION(3,0);
	case EVDF_PIXEL_SHADER_1_1:
		return Caps.PixelShaderVersion >= D3DPS_VERSION(1,1);
	case EVDF_PIXEL_SHADER_1_2:
		return Caps.PixelShaderVersion >= D3DPS_VERSION(1,2);
	case EVDF_PIXEL_SHADER_1_3:
		return Caps.PixelShaderVersion >= D3DPS_VERSION(1,3);
	case EVDF_PIXEL_SHADER_1_4:
		return Caps.PixelShaderVersion >= D3DPS_VERSION(1,4);
	case EVDF_PIXEL_SHADER_2_0:
		return Caps.PixelShaderVersion >= D3DPS_VERSION(2,0);
	case EVDF_PIXEL_SHADER_3_0:
		return Caps.PixelShaderVersion >= D3DPS_VERSION(3,0);
	case EVDF_HLSL:
		return Caps.VertexShaderVersion >= D3DVS_VERSION(1,1);
	case EVDF_TEXTURE_NSQUARE:
		return (Caps.TextureCaps & D3DPTEXTURECAPS_SQUAREONLY) == 0;
	case EVDF_TEXTURE_NPOT:
		return (Caps.TextureCaps & D3DPTEXTURECAPS_POW2) == 0;
	case EVDF_COLOR_MASK:
		return (Caps.PrimitiveMiscCaps & D3DPMISCCAPS_COLORWRITEENABLE) != 0;
	case EVDF_MULTIPLE_RENDER_TARGETS:
		return Caps.NumSimultaneousRTs > 1;
	case EVDF_MRT_COLOR_MASK:
		return (Caps.PrimitiveMiscCaps & D3DPMISCCAPS_INDEPENDENTWRITEMASKS) != 0;
	case EVDF_MRT_BLEND:
		return (Caps.PrimitiveMiscCaps & D3DPMISCCAPS_MRTPOSTPIXELSHADERBLENDING) != 0;
	case EVDF_OCCLUSION_QUERY:
		return OcclusionQuerySupport;
	case EVDF_POLYGON_OFFSET:
		return (Caps.RasterCaps & (D3DPRASTERCAPS_DEPTHBIAS|D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS)) != 0;
	case EVDF_BLEND_OPERATIONS:
	case EVDF_TEXTURE_MATRIX:
#ifdef _IRR_COMPILE_WITH_CG_
	// available iff. define is present
	case EVDF_CG:
#endif
		return true;
	default:
		return false;
	};
}


//! sets transformation
void CD3D9Driver::setTransform(E_TRANSFORMATION_STATE state,
		const core::matrix4& mat)
{
	Transformation3DChanged = true;

	switch(state)
	{
	case ETS_VIEW:
		pID3DDevice->SetTransform(D3DTS_VIEW, (D3DMATRIX*)((void*)mat.pointer()));
		break;
	case ETS_WORLD:
		pID3DDevice->SetTransform(D3DTS_WORLD, (D3DMATRIX*)((void*)mat.pointer()));
		break;
	case ETS_PROJECTION:
		pID3DDevice->SetTransform( D3DTS_PROJECTION, (D3DMATRIX*)((void*)mat.pointer()));
		break;
	case ETS_COUNT:
		return;
	default:
		if (state-ETS_TEXTURE_0 < MATERIAL_MAX_TEXTURES)
		{
			if (mat.isIdentity())
				pID3DDevice->SetTextureStageState( state - ETS_TEXTURE_0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
			else
			{
				pID3DDevice->SetTextureStageState( state - ETS_TEXTURE_0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2 );
				pID3DDevice->SetTransform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0+ ( state - ETS_TEXTURE_0 )),
					(D3DMATRIX*)((void*)mat.pointer()));
			}
		}
		break;
	}

	Matrices[state] = mat;
}


//! sets the current Texture
bool CD3D9Driver::setActiveTexture(u32 stage, const video::ITexture* texture)
{
	if (CurrentTexture[stage] == texture)
		return true;

	if (texture && texture->getDriverType() != EDT_DIRECT3D9)
	{
		os::Printer::log("Fatal Error: Tried to set a texture not owned by this driver.", ELL_ERROR);
		return false;
	}

	CurrentTexture[stage] = texture;

	if (!texture)
	{
		pID3DDevice->SetTexture(stage, 0);
		pID3DDevice->SetTextureStageState( stage, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE );
	}
	else
	{
		pID3DDevice->SetTexture(stage, ((const CD3D9Texture*)texture)->getDX9Texture());
	}
	return true;
}


//! sets a material
void CD3D9Driver::setMaterial(const SMaterial& material)
{
	Material = material;
	OverrideMaterial.apply(Material);

	for (u32 i=0; i<MaxTextureUnits; ++i)
	{
		setActiveTexture(i, Material.getTexture(i));
		setTransform((E_TRANSFORMATION_STATE) ( ETS_TEXTURE_0 + i ),
				material.getTextureMatrix(i));
	}
}


//! returns a device dependent texture from a software surface (IImage)
video::ITexture* CD3D9Driver::createDeviceDependentTexture(IImage* surface,const io::path& name, void* mipmapData)
{
	return new CD3D9Texture(surface, this, TextureCreationFlags, name, mipmapData);
}


//! Enables or disables a texture creation flag.
void CD3D9Driver::setTextureCreationFlag(E_TEXTURE_CREATION_FLAG flag,
		bool enabled)
{
	if (flag == video::ETCF_CREATE_MIP_MAPS && !queryFeature(EVDF_MIP_MAP))
		enabled = false;

	CNullDriver::setTextureCreationFlag(flag, enabled);
}


//! sets a render target
bool CD3D9Driver::setRenderTarget(video::ITexture* texture,
		bool clearBackBuffer, bool clearZBuffer, SColor color)
{
	// check for right driver type

	if (texture && texture->getDriverType() != EDT_DIRECT3D9)
	{
		os::Printer::log("Fatal Error: Tried to set a texture not owned by this driver.", ELL_ERROR);
		return false;
	}

	// check for valid render target

	if (texture && !texture->isRenderTarget())
	{
		os::Printer::log("Fatal Error: Tried to set a non render target texture as render target.", ELL_ERROR);
		return false;
	}

	CD3D9Texture* tex = static_cast<CD3D9Texture*>(texture);

	// check if we should set the previous RT back

	bool ret = true;

	for(u32 i = 1; i < NumSetMRTs; i++)
	{
		// First texture handled elsewhere
		pID3DDevice->SetRenderTarget(i, NULL);
	}
	if (tex == 0)
	{
		if (PrevRenderTarget)
		{
			if (FAILED(pID3DDevice->SetRenderTarget(0, PrevRenderTarget)))
			{
				os::Printer::log("Error: Could not set back to previous render target.", ELL_ERROR);
				ret = false;
			}
			if (FAILED(pID3DDevice->SetDepthStencilSurface(DepthBuffers[0]->Surface)))
			{
				os::Printer::log("Error: Could not set main depth buffer.", ELL_ERROR);
			}

			CurrentRendertargetSize = core::dimension2d<u32>(0,0);
			PrevRenderTarget->Release();
			PrevRenderTarget = 0;
		}
	}
	else
	{
		// we want to set a new target. so do this.

		// store previous target

		if (!PrevRenderTarget)
		{
			if (FAILED(pID3DDevice->GetRenderTarget(0, &PrevRenderTarget)))
			{
				os::Printer::log("Could not get previous render target.", ELL_ERROR);
				return false;
			}
		}

		// set new render target

		if (FAILED(pID3DDevice->SetRenderTarget(0, tex->getRenderTargetSurface())))
		{
			os::Printer::log("Error: Could not set render target.", ELL_ERROR);
			return false;
		}
		CurrentRendertargetSize = tex->getSize();

		if (FAILED(pID3DDevice->SetDepthStencilSurface(tex->DepthSurface->Surface)))
		{
			os::Printer::log("Error: Could not set new depth buffer.", ELL_ERROR);
		}
	}
	Transformation3DChanged=true;

	if (clearBackBuffer || clearZBuffer)
	{
		DWORD flags = 0;

		if (clearBackBuffer)
			flags |= D3DCLEAR_TARGET;

		if (clearZBuffer)
			flags |= D3DCLEAR_ZBUFFER;

		pID3DDevice->Clear(0, NULL, flags, color.color, 1.0f, 0);
	}

	return ret;
}


//! Sets multiple render targets
bool CD3D9Driver::setRenderTarget(const core::array<video::IRenderTarget>& targets,
				bool clearBackBuffer, bool clearZBuffer, SColor color)
{
	if (targets.size()==0)
		return setRenderTarget(0, clearBackBuffer, clearZBuffer, color);

	u32 maxMultipleRTTs = core::min_(MaxMRTs, targets.size());

	for (u32 i = 0; i < maxMultipleRTTs; ++i)
	{
		if (targets[i].TargetType != ERT_RENDER_TEXTURE || !targets[i].RenderTexture)
		{
			maxMultipleRTTs = i;
			os::Printer::log("Missing texture for MRT.", ELL_WARNING);
			break;
		}

		// check for right driver type

		if (targets[i].RenderTexture->getDriverType() != EDT_DIRECT3D9)
		{
			maxMultipleRTTs = i;
			os::Printer::log("Tried to set a texture not owned by this driver.", ELL_WARNING);
			break;
		}

		// check for valid render target

		if (!targets[i].RenderTexture->isRenderTarget())
		{
			maxMultipleRTTs = i;
			os::Printer::log("Tried to set a non render target texture as render target.", ELL_WARNING);
			break;
		}

		// check for valid size

		if (targets[0].RenderTexture->getSize() != targets[i].RenderTexture->getSize())
		{
			maxMultipleRTTs = i;
			os::Printer::log("Render target texture has wrong size.", ELL_WARNING);
			break;
		}
	}
	if (maxMultipleRTTs==0)
	{
		os::Printer::log("Fatal Error: No valid MRT found.", ELL_ERROR);
		return false;
	}

	CD3D9Texture* tex = static_cast<CD3D9Texture*>(targets[0].RenderTexture);

	// check if we should set the previous RT back

	bool ret = true;

	// we want to set a new target. so do this.
	// store previous target

	if (!PrevRenderTarget)
	{
		if (FAILED(pID3DDevice->GetRenderTarget(0, &PrevRenderTarget)))
		{
			os::Printer::log("Could not get previous render target.", ELL_ERROR);
			return false;
		}
	}

	// set new render target

	// In d3d9 we have at most 4 MRTs, so the following is enough
	D3DRENDERSTATETYPE colorWrite[4]={D3DRS_COLORWRITEENABLE, D3DRS_COLORWRITEENABLE1, D3DRS_COLORWRITEENABLE2, D3DRS_COLORWRITEENABLE3};
	for (u32 i = 0; i < maxMultipleRTTs; ++i)
	{
		if (FAILED(pID3DDevice->SetRenderTarget(i, static_cast<CD3D9Texture*>(targets[i].RenderTexture)->getRenderTargetSurface())))
		{
			os::Printer::log("Error: Could not set render target.", ELL_ERROR);
			return false;
		}
		if (i<4 && (i==0 || queryFeature(EVDF_MRT_COLOR_MASK)))
		{
			const DWORD flag =
				((targets[i].ColorMask & ECP_RED)?D3DCOLORWRITEENABLE_RED:0) |
				((targets[i].ColorMask & ECP_GREEN)?D3DCOLORWRITEENABLE_GREEN:0) |
				((targets[i].ColorMask & ECP_BLUE)?D3DCOLORWRITEENABLE_BLUE:0) |
				((targets[i].ColorMask & ECP_ALPHA)?D3DCOLORWRITEENABLE_ALPHA:0);
			pID3DDevice->SetRenderState(colorWrite[i], flag);
		}
	}
	for(u32 i = maxMultipleRTTs; i < NumSetMRTs; i++)
	{
		pID3DDevice->SetRenderTarget(i, NULL);
	}
	NumSetMRTs=maxMultipleRTTs;

	CurrentRendertargetSize = tex->getSize();

	if (FAILED(pID3DDevice->SetDepthStencilSurface(tex->DepthSurface->Surface)))
	{
		os::Printer::log("Error: Could not set new depth buffer.", ELL_ERROR);
	}

	if (clearBackBuffer || clearZBuffer)
	{
		DWORD flags = 0;

		if (clearBackBuffer)
			flags |= D3DCLEAR_TARGET;

		if (clearZBuffer)
			flags |= D3DCLEAR_ZBUFFER;

		pID3DDevice->Clear(0, NULL, flags, color.color, 1.0f, 0);
	}

	return ret;
}


//! sets a viewport
void CD3D9Driver::setViewPort(const core::rect<s32>& area)
{
	core::rect<s32> vp = area;
	core::rect<s32> rendert(0,0, getCurrentRenderTargetSize().Width, getCurrentRenderTargetSize().Height);
	vp.clipAgainst(rendert);
	if (vp.getHeight()>0 && vp.getWidth()>0)
	{
		D3DVIEWPORT9 viewPort;
		viewPort.X = vp.UpperLeftCorner.X;
		viewPort.Y = vp.UpperLeftCorner.Y;
		viewPort.Width = vp.getWidth();
		viewPort.Height = vp.getHeight();
		viewPort.MinZ = 0.0f;
		viewPort.MaxZ = 1.0f;

		HRESULT hr = pID3DDevice->SetViewport(&viewPort);
		if (FAILED(hr))
			os::Printer::log("Failed setting the viewport.", ELL_WARNING);
		else
			ViewPort = vp;
	}
}


//! gets the area of the current viewport
const core::rect<s32>& CD3D9Driver::getViewPort() const
{
	return ViewPort;
}


bool CD3D9Driver::updateVertexHardwareBuffer(SHWBufferLink_d3d9 *hwBuffer)
{
	if (!hwBuffer)
		return false;

	const scene::IMeshBuffer* mb = hwBuffer->MeshBuffer;
	const void* vertices=mb->getVertices();
	const u32 vertexCount=mb->getVertexCount();
	const E_VERTEX_TYPE vType=mb->getVertexType();
	const u32 vertexSize = getVertexPitchFromType(vType);
	const u32 bufSize = vertexSize * vertexCount;

	if (!hwBuffer->vertexBuffer || (bufSize > hwBuffer->vertexBufferSize))
	{
		if (hwBuffer->vertexBuffer)
		{
			hwBuffer->vertexBuffer->Release();
			hwBuffer->vertexBuffer=0;
		}

		DWORD FVF;
		// Get the vertex sizes and cvf
		switch (vType)
		{
			case EVT_STANDARD:
				FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1;
				break;
			case EVT_2TCOORDS:
				FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX2;
				break;
			case EVT_TANGENTS:
				FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX3;
				break;
			default:
				return false;
		}

		DWORD flags = D3DUSAGE_WRITEONLY; // SIO2: Default to D3DUSAGE_WRITEONLY
		if (hwBuffer->Mapped_Vertex != scene::EHM_STATIC)
			flags |= D3DUSAGE_DYNAMIC;

		if (FAILED(pID3DDevice->CreateVertexBuffer(bufSize, flags, FVF, D3DPOOL_DEFAULT, &hwBuffer->vertexBuffer, NULL)))
			return false;
		hwBuffer->vertexBufferSize = bufSize;

		flags = 0; // SIO2: Reset flags before Lock
		if (hwBuffer->Mapped_Vertex != scene::EHM_STATIC)
			flags = D3DLOCK_DISCARD;

		void* lockedBuffer = 0;
		hwBuffer->vertexBuffer->Lock(0, bufSize, (void**)&lockedBuffer, flags);
		memcpy(lockedBuffer, vertices, bufSize);
		hwBuffer->vertexBuffer->Unlock();
	}
	else
	{
		void* lockedBuffer = 0;
		hwBuffer->vertexBuffer->Lock(0, bufSize, (void**)&lockedBuffer, D3DLOCK_DISCARD);
		memcpy(lockedBuffer, vertices, bufSize);
		hwBuffer->vertexBuffer->Unlock();
	}

	return true;
}


bool CD3D9Driver::updateIndexHardwareBuffer(SHWBufferLink_d3d9 *hwBuffer)
{
	if (!hwBuffer)
		return false;

	const scene::IMeshBuffer* mb = hwBuffer->MeshBuffer;
	const u16* indices=mb->getIndices();
	const u32 indexCount=mb->getIndexCount();
	u32 indexSize = 2;
	D3DFORMAT indexType=D3DFMT_UNKNOWN;
	switch (mb->getIndexType())
	{
		case EIT_16BIT:
		{
			indexType=D3DFMT_INDEX16;
			indexSize = 2;
			break;
		}
		case EIT_32BIT:
		{
			indexType=D3DFMT_INDEX32;
			indexSize = 4;
			break;
		}
	}

	const u32 bufSize = indexSize * indexCount;
	if (!hwBuffer->indexBuffer || (bufSize > hwBuffer->indexBufferSize))
	{
		if (hwBuffer->indexBuffer)
		{
			hwBuffer->indexBuffer->Release();
			hwBuffer->indexBuffer=0;
		}

		DWORD flags = D3DUSAGE_WRITEONLY; // SIO2: Default to D3DUSAGE_WRITEONLY
		if (hwBuffer->Mapped_Index != scene::EHM_STATIC)
			flags |= D3DUSAGE_DYNAMIC; // SIO2: Add DYNAMIC flag for dynamic buffer data

		if (FAILED(pID3DDevice->CreateIndexBuffer(bufSize, flags, indexType, D3DPOOL_DEFAULT, &hwBuffer->indexBuffer, NULL)))
			return false;

		flags = 0; // SIO2: Reset flags before Lock
		if (hwBuffer->Mapped_Index != scene::EHM_STATIC)
			flags = D3DLOCK_DISCARD;

		void* lockedBuffer = 0;
		if (FAILED(hwBuffer->indexBuffer->Lock( 0, 0, (void**)&lockedBuffer, flags)))
			return false;

		memcpy(lockedBuffer, indices, bufSize);
		hwBuffer->indexBuffer->Unlock();

		hwBuffer->indexBufferSize = bufSize;
	}
	else
	{
		void* lockedBuffer = 0;
		if( SUCCEEDED(hwBuffer->indexBuffer->Lock( 0, 0, (void**)&lockedBuffer, D3DLOCK_DISCARD)))
		{
			memcpy(lockedBuffer, indices, bufSize);
			hwBuffer->indexBuffer->Unlock();
		}
	}

	return true;
}


//! updates hardware buffer if needed
bool CD3D9Driver::updateHardwareBuffer(SHWBufferLink *hwBuffer)
{
	if (!hwBuffer)
		return false;

	if (hwBuffer->Mapped_Vertex!=scene::EHM_NEVER)
	{
		if (hwBuffer->ChangedID_Vertex != hwBuffer->MeshBuffer->getChangedID_Vertex()
			|| !((SHWBufferLink_d3d9*)hwBuffer)->vertexBuffer)
		{
			hwBuffer->ChangedID_Vertex = hwBuffer->MeshBuffer->getChangedID_Vertex();

			if (!updateVertexHardwareBuffer((SHWBufferLink_d3d9*)hwBuffer))
				return false;
		}
	}

	if (hwBuffer->Mapped_Index!=scene::EHM_NEVER)
	{
		if (hwBuffer->ChangedID_Index != hwBuffer->MeshBuffer->getChangedID_Index()
			|| !((SHWBufferLink_d3d9*)hwBuffer)->indexBuffer)
		{
			hwBuffer->ChangedID_Index = hwBuffer->MeshBuffer->getChangedID_Index();

			if (!updateIndexHardwareBuffer((SHWBufferLink_d3d9*)hwBuffer))
				return false;
		}
	}

	return true;
}


//! Create hardware buffer from meshbuffer
CD3D9Driver::SHWBufferLink *CD3D9Driver::createHardwareBuffer(const scene::IMeshBuffer* mb)
{
	// Looks like d3d does not support only partial buffering, so refuse
	// in any case of NEVER
	if (!mb || (mb->getHardwareMappingHint_Index()==scene::EHM_NEVER || mb->getHardwareMappingHint_Vertex()==scene::EHM_NEVER))
		return 0;

	SHWBufferLink_d3d9 *hwBuffer=new SHWBufferLink_d3d9(mb);

	//add to map
	HWBufferMap.insert(hwBuffer->MeshBuffer, hwBuffer);

	hwBuffer->ChangedID_Vertex=hwBuffer->MeshBuffer->getChangedID_Vertex();
	hwBuffer->ChangedID_Index=hwBuffer->MeshBuffer->getChangedID_Index();
	hwBuffer->Mapped_Vertex=mb->getHardwareMappingHint_Vertex();
	hwBuffer->Mapped_Index=mb->getHardwareMappingHint_Index();
	hwBuffer->LastUsed=0;
	hwBuffer->vertexBuffer=0;
	hwBuffer->indexBuffer=0;
	hwBuffer->vertexBufferSize=0;
	hwBuffer->indexBufferSize=0;

	if (!updateHardwareBuffer(hwBuffer))
	{
		deleteHardwareBuffer(hwBuffer);
		return 0;
	}

	return hwBuffer;
}


void CD3D9Driver::deleteHardwareBuffer(SHWBufferLink *_HWBuffer)
{
	if (!_HWBuffer)
		return;

	SHWBufferLink_d3d9 *HWBuffer=(SHWBufferLink_d3d9*)_HWBuffer;
	if (HWBuffer->indexBuffer)
	{
		HWBuffer->indexBuffer->Release();
		HWBuffer->indexBuffer = 0;
	}

	if (HWBuffer->vertexBuffer)
	{
		HWBuffer->vertexBuffer->Release();
		HWBuffer->vertexBuffer = 0;
	}

	CNullDriver::deleteHardwareBuffer(_HWBuffer);
}


//! Draw hardware buffer
void CD3D9Driver::drawHardwareBuffer(SHWBufferLink *_HWBuffer)
{
	if (!_HWBuffer)
		return;

	SHWBufferLink_d3d9 *HWBuffer=(SHWBufferLink_d3d9*)_HWBuffer;

	updateHardwareBuffer(HWBuffer); //check if update is needed

	HWBuffer->LastUsed=0;//reset count

	const scene::IMeshBuffer* mb = HWBuffer->MeshBuffer;
	const E_VERTEX_TYPE vType = mb->getVertexType();
	const u32 stride = getVertexPitchFromType(vType);
	const void* vPtr = mb->getVertices();
	const void* iPtr = mb->getIndices();
	if (HWBuffer->vertexBuffer)
	{
		pID3DDevice->SetStreamSource(0, HWBuffer->vertexBuffer, 0, stride);
		vPtr=0;
	}
	if (HWBuffer->indexBuffer)
	{
		pID3DDevice->SetIndices(HWBuffer->indexBuffer);
		iPtr=0;
	}

	drawVertexPrimitiveList(vPtr, mb->getVertexCount(), iPtr, mb->getIndexCount()/3, mb->getVertexType(), scene::EPT_TRIANGLES, mb->getIndexType());

	if (HWBuffer->vertexBuffer)
		pID3DDevice->SetStreamSource(0, 0, 0, 0);
	if (HWBuffer->indexBuffer)
		pID3DDevice->SetIndices(0);
}


//! Create occlusion query.
/** Use node for identification and mesh for occlusion test. */
void CD3D9Driver::addOcclusionQuery(scene::ISceneNode* node,
		const scene::IMesh* mesh)
{
	if (!queryFeature(EVDF_OCCLUSION_QUERY))
		return;
	CNullDriver::addOcclusionQuery(node, mesh);
	const s32 index = OcclusionQueries.linear_search(SOccQuery(node));
	if ((index != -1) && (OcclusionQueries[index].PID == 0))
		pID3DDevice->CreateQuery(D3DQUERYTYPE_OCCLUSION, reinterpret_cast<IDirect3DQuery9**>(&OcclusionQueries[index].PID));
}


//! Remove occlusion query.
void CD3D9Driver::removeOcclusionQuery(scene::ISceneNode* node)
{
	const s32 index = OcclusionQueries.linear_search(SOccQuery(node));
	if (index != -1)
	{
		if (OcclusionQueries[index].PID != 0)
			reinterpret_cast<IDirect3DQuery9*>(OcclusionQueries[index].PID)->Release();
		CNullDriver::removeOcclusionQuery(node);
	}
}


//! Run occlusion query. Draws mesh stored in query.
/** If the mesh shall not be rendered visible, use
overrideMaterial to disable the color and depth buffer. */
void CD3D9Driver::runOcclusionQuery(scene::ISceneNode* node, bool visible)
{
	if (!node)
		return;

	const s32 index = OcclusionQueries.linear_search(SOccQuery(node));
	if (index != -1)
	{
		if (OcclusionQueries[index].PID)
			reinterpret_cast<IDirect3DQuery9*>(OcclusionQueries[index].PID)->Issue(D3DISSUE_BEGIN);
		CNullDriver::runOcclusionQuery(node,visible);
		if (OcclusionQueries[index].PID)
			reinterpret_cast<IDirect3DQuery9*>(OcclusionQueries[index].PID)->Issue(D3DISSUE_END);
	}
}


//! Update occlusion query. Retrieves results from GPU.
/** If the query shall not block, set the flag to false.
Update might not occur in this case, though */
void CD3D9Driver::updateOcclusionQuery(scene::ISceneNode* node, bool block)
{
	const s32 index = OcclusionQueries.linear_search(SOccQuery(node));
	if (index != -1)
	{
		// not yet started
		if (OcclusionQueries[index].Run==u32(~0))
			return;
		bool available = block?true:false;
		int tmp=0;
		if (!block)
			available=(reinterpret_cast<IDirect3DQuery9*>(OcclusionQueries[index].PID)->GetData(&tmp, sizeof(DWORD), 0)==S_OK);
		else
		{
			do
			{
				HRESULT hr = reinterpret_cast<IDirect3DQuery9*>(OcclusionQueries[index].PID)->GetData(&tmp, sizeof(DWORD), D3DGETDATA_FLUSH);
				available = (hr == S_OK);
				if (hr!=S_FALSE)
					break;
			} while (!available);
		}
		if (available)
			OcclusionQueries[index].Result = tmp;
	}
}


//! Return query result.
/** Return value is the number of visible pixels/fragments.
The value is a safe approximation, i.e. can be larger than the
actual value of pixels. */
u32 CD3D9Driver::getOcclusionQueryResult(scene::ISceneNode* node) const
{
	const s32 index = OcclusionQueries.linear_search(SOccQuery(node));
	if (index != -1)
		return OcclusionQueries[index].Result;
	else
		return ~0;
}


//! draws a vertex primitive list
void CD3D9Driver::drawVertexPrimitiveList(const void* vertices,
		u32 vertexCount, const void* indexList, u32 primitiveCount,
		E_VERTEX_TYPE vType, scene::E_PRIMITIVE_TYPE pType,
		E_INDEX_TYPE iType)
{
	if (!checkPrimitiveCount(primitiveCount))
		return;

	CNullDriver::drawVertexPrimitiveList(vertices, vertexCount, indexList, primitiveCount, vType, pType,iType);

	if (!vertexCount || !primitiveCount)
		return;

	draw2D3DVertexPrimitiveList(vertices, vertexCount, indexList, primitiveCount,
		vType, pType, iType, true);
}


//! draws a vertex primitive list
void CD3D9Driver::draw2DVertexPrimitiveList(const void* vertices,
		u32 vertexCount, const void* indexList, u32 primitiveCount,
		E_VERTEX_TYPE vType, scene::E_PRIMITIVE_TYPE pType,
		E_INDEX_TYPE iType)
{
	if (!checkPrimitiveCount(primitiveCount))
		return;

	CNullDriver::draw2DVertexPrimitiveList(vertices, vertexCount, indexList, primitiveCount, vType, pType,iType);

	if (!vertexCount || !primitiveCount)
		return;

	draw2D3DVertexPrimitiveList(vertices, vertexCount, indexList, primitiveCount,
		vType, pType, iType, false);
}


void CD3D9Driver::draw2D3DVertexPrimitiveList(const void* vertices,
		u32 vertexCount, const void* indexList, u32 primitiveCount,
		E_VERTEX_TYPE vType, scene::E_PRIMITIVE_TYPE pType,
		E_INDEX_TYPE iType, bool is3D)
{
	setVertexShader(vType);

	const u32 stride = getVertexPitchFromType(vType);

	D3DFORMAT indexType=D3DFMT_UNKNOWN;
	switch (iType)
	{
		case (EIT_16BIT):
		{
			indexType=D3DFMT_INDEX16;
			break;
		}
		case (EIT_32BIT):
		{
			indexType=D3DFMT_INDEX32;
			break;
		}
	}

	if (is3D)
	{
	       	if (!setRenderStates3DMode())
			return;
	}
	else
	{
		if (Material.MaterialType==EMT_ONETEXTURE_BLEND)
		{
			E_BLEND_FACTOR srcFact;
			E_BLEND_FACTOR dstFact;
			E_MODULATE_FUNC modulo;
			u32 alphaSource;
			unpack_textureBlendFunc ( srcFact, dstFact, modulo, alphaSource, Material.MaterialTypeParam);
			setRenderStates2DMode(alphaSource&video::EAS_VERTEX_COLOR, (Material.getTexture(0) != 0), (alphaSource&video::EAS_TEXTURE) != 0);
		}
		else
			setRenderStates2DMode(Material.MaterialType==EMT_TRANSPARENT_VERTEX_ALPHA, (Material.getTexture(0) != 0), Material.MaterialType==EMT_TRANSPARENT_ALPHA_CHANNEL);
	}

	switch (pType)
	{
		case scene::EPT_POINT_SPRITES:
		case scene::EPT_POINTS:
		{
			f32 tmp=Material.Thickness/getScreenSize().Height;
			if (pType==scene::EPT_POINT_SPRITES)
				pID3DDevice->SetRenderState(D3DRS_POINTSPRITEENABLE, TRUE);
			pID3DDevice->SetRenderState(D3DRS_POINTSCALEENABLE, TRUE);
			pID3DDevice->SetRenderState(D3DRS_POINTSIZE, F2DW(tmp));
			tmp=1.0f;
			pID3DDevice->SetRenderState(D3DRS_POINTSCALE_A, F2DW(tmp));
			pID3DDevice->SetRenderState(D3DRS_POINTSCALE_B, F2DW(tmp));
			pID3DDevice->SetRenderState(D3DRS_POINTSIZE_MIN, F2DW(tmp));
			tmp=0.0f;
			pID3DDevice->SetRenderState(D3DRS_POINTSCALE_C, F2DW(tmp));

			if (!vertices)
			{
				pID3DDevice->DrawIndexedPrimitive(D3DPT_POINTLIST, 0, 0, vertexCount, 0, primitiveCount);
			}
			else
			{
				pID3DDevice->DrawIndexedPrimitiveUP(D3DPT_POINTLIST, 0, vertexCount,
				primitiveCount, indexList, indexType, vertices, stride);
			}

			pID3DDevice->SetRenderState(D3DRS_POINTSCALEENABLE, FALSE);
			if (pType==scene::EPT_POINT_SPRITES)
				pID3DDevice->SetRenderState(D3DRS_POINTSPRITEENABLE, FALSE);
		}
			break;
		case scene::EPT_LINE_STRIP:
			if(!vertices)
				pID3DDevice->DrawIndexedPrimitive(D3DPT_LINESTRIP, 0, 0, vertexCount, 0, primitiveCount);
			else
				pID3DDevice->DrawIndexedPrimitiveUP(D3DPT_LINESTRIP, 0, vertexCount,
					primitiveCount, indexList, indexType, vertices, stride);
			break;
		case scene::EPT_LINE_LOOP:
			if(!vertices)
			{
				// TODO: Implement proper hardware support for this primitive type.
				// (No looping occurs currently because this would require a way to
				// draw the hardware buffer with a custom set of indices. We may even
				// need to create a new mini index buffer specifically for this
				// primitive type.)
				pID3DDevice->DrawIndexedPrimitive(D3DPT_LINELIST, 0, 0, vertexCount, 0, primitiveCount);
			}
			else
			{
				pID3DDevice->DrawIndexedPrimitiveUP(D3DPT_LINESTRIP, 0, vertexCount,
				primitiveCount - 1, indexList, indexType, vertices, stride);

				u16 tmpIndices[] = {primitiveCount - 1, 0};

				pID3DDevice->DrawIndexedPrimitiveUP(D3DPT_LINELIST, 0, vertexCount,
					1, tmpIndices, indexType, vertices, stride);
			}
			break;
		case scene::EPT_LINES:
			if(!vertices)
				pID3DDevice->DrawIndexedPrimitive(D3DPT_LINELIST, 0, 0, vertexCount, 0, primitiveCount);
			else
				pID3DDevice->DrawIndexedPrimitiveUP(D3DPT_LINELIST, 0, vertexCount,
					primitiveCount, indexList, indexType, vertices, stride);
			break;
		case scene::EPT_TRIANGLE_STRIP:
			if(!vertices)
				pID3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP, 0, 0, vertexCount, 0, primitiveCount);
			else
				pID3DDevice->DrawIndexedPrimitiveUP(D3DPT_TRIANGLESTRIP, 0, vertexCount, primitiveCount,
						indexList, indexType, vertices, stride);
			break;
		case scene::EPT_TRIANGLE_FAN:
			if(!vertices)
				pID3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLEFAN, 0, 0, vertexCount, 0, primitiveCount);
			else
				pID3DDevice->DrawIndexedPrimitiveUP(D3DPT_TRIANGLEFAN, 0, vertexCount, primitiveCount,
						indexList, indexType, vertices, stride);
				break;
		case scene::EPT_TRIANGLES:
			if(!vertices)
			{
				pID3DDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, vertexCount, 0, primitiveCount);
			}
			else
			{
				pID3DDevice->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, vertexCount,
					primitiveCount, indexList, indexType, vertices, stride);
			}
			break;
	}
}


void CD3D9Driver::draw2DImage(const video::ITexture* texture,
		const core::rect<s32>& destRect,
		const core::rect<s32>& sourceRect,
		const core::rect<s32>* clipRect,
		const video::SColor* const colors,
		bool useAlphaChannelOfTexture)
{
	if(!texture)
		return;

	const core::dimension2d<u32>& ss = texture->getOriginalSize();
	core::rect<f32> tcoords;
	tcoords.UpperLeftCorner.X = (f32)sourceRect.UpperLeftCorner.X / (f32)ss.Width;
	tcoords.UpperLeftCorner.Y = (f32)sourceRect.UpperLeftCorner.Y / (f32)ss.Height;
	tcoords.LowerRightCorner.X = (f32)sourceRect.LowerRightCorner.X / (f32)ss.Width;
	tcoords.LowerRightCorner.Y = (f32)sourceRect.LowerRightCorner.Y / (f32)ss.Height;

	const core::dimension2d<u32>& renderTargetSize = getCurrentRenderTargetSize();

	const video::SColor temp[4] =
	{
		0xFFFFFFFF,
		0xFFFFFFFF,
		0xFFFFFFFF,
		0xFFFFFFFF
	};

	const video::SColor* const useColor = colors ? colors : temp;

	S3DVertex vtx[4]; // clock wise
	vtx[0] = S3DVertex((f32)destRect.UpperLeftCorner.X, (f32)destRect.UpperLeftCorner.Y, 0.0f,
			0.0f, 0.0f, 0.0f, useColor[0],
			tcoords.UpperLeftCorner.X, tcoords.UpperLeftCorner.Y);
	vtx[1] = S3DVertex((f32)destRect.LowerRightCorner.X, (f32)destRect.UpperLeftCorner.Y, 0.0f,
			0.0f, 0.0f, 0.0f, useColor[3],
			tcoords.LowerRightCorner.X, tcoords.UpperLeftCorner.Y);
	vtx[2] = S3DVertex((f32)destRect.LowerRightCorner.X, (f32)destRect.LowerRightCorner.Y, 0.0f,
			0.0f, 0.0f, 0.0f, useColor[2],
			tcoords.LowerRightCorner.X, tcoords.LowerRightCorner.Y);
	vtx[3] = S3DVertex((f32)destRect.UpperLeftCorner.X, (f32)destRect.LowerRightCorner.Y, 0.0f,
			0.0f, 0.0f, 0.0f, useColor[1],
			tcoords.UpperLeftCorner.X, tcoords.LowerRightCorner.Y);

	s16 indices[6] = {0,1,2,0,2,3};

	setActiveTexture(0, const_cast<video::ITexture*>(texture));

	setRenderStates2DMode(useColor[0].getAlpha()<255 || useColor[1].getAlpha()<255 ||
			useColor[2].getAlpha()<255 || useColor[3].getAlpha()<255,
			true, useAlphaChannelOfTexture);

	setVertexShader(EVT_STANDARD);

	if (clipRect)
	{
		pID3DDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
		RECT scissor;
		scissor.left = clipRect->UpperLeftCorner.X;
		scissor.top = clipRect->UpperLeftCorner.Y;
		scissor.right = clipRect->LowerRightCorner.X;
		scissor.bottom = clipRect->LowerRightCorner.Y;
		pID3DDevice->SetScissorRect(&scissor);
	}

	pID3DDevice->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 4, 2, &indices[0],
				D3DFMT_INDEX16,&vtx[0], sizeof(S3DVertex));

	if (clipRect)
		pID3DDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
}


void CD3D9Driver::draw2DImageBatch(const video::ITexture* texture,
				const core::array<core::position2d<s32> >& positions,
				const core::array<core::rect<s32> >& sourceRects,
				const core::rect<s32>* clipRect,
				SColor color,
				bool useAlphaChannelOfTexture)
{
	if (!texture)
		return;

	if (!setActiveTexture(0, const_cast<video::ITexture*>(texture)))
		return;

	setRenderStates2DMode(color.getAlpha()<255, true, useAlphaChannelOfTexture);

	const irr::u32 drawCount = core::min_<u32>(positions.size(), sourceRects.size());

	core::array<S3DVertex> vtx(drawCount * 4);
	core::array<u16> indices(drawCount * 6);

	for(u32 i = 0;i < drawCount;i++)
	{
		core::position2d<s32> targetPos = positions[i];
		core::position2d<s32> sourcePos = sourceRects[i].UpperLeftCorner;
		// This needs to be signed as it may go negative.
		core::dimension2d<s32> sourceSize(sourceRects[i].getSize());

		if (clipRect)
		{
			if (targetPos.X < clipRect->UpperLeftCorner.X)
			{
				sourceSize.Width += targetPos.X - clipRect->UpperLeftCorner.X;
				if (sourceSize.Width <= 0)
					continue;

				sourcePos.X -= targetPos.X - clipRect->UpperLeftCorner.X;
				targetPos.X = clipRect->UpperLeftCorner.X;
			}

			if (targetPos.X + (s32)sourceSize.Width > clipRect->LowerRightCorner.X)
			{
				sourceSize.Width -= (targetPos.X + sourceSize.Width) - clipRect->LowerRightCorner.X;
				if (sourceSize.Width <= 0)
					continue;
			}

			if (targetPos.Y < clipRect->UpperLeftCorner.Y)
			{
				sourceSize.Height += targetPos.Y - clipRect->UpperLeftCorner.Y;
				if (sourceSize.Height <= 0)
					continue;

				sourcePos.Y -= targetPos.Y - clipRect->UpperLeftCorner.Y;
				targetPos.Y = clipRect->UpperLeftCorner.Y;
			}

			if (targetPos.Y + (s32)sourceSize.Height > clipRect->LowerRightCorner.Y)
			{
				sourceSize.Height -= (targetPos.Y + sourceSize.Height) - clipRect->LowerRightCorner.Y;
				if (sourceSize.Height <= 0)
					continue;
			}
		}

		// clip these coordinates

		if (targetPos.X<0)
		{
			sourceSize.Width += targetPos.X;
			if (sourceSize.Width <= 0)
				continue;

			sourcePos.X -= targetPos.X;
			targetPos.X = 0;
		}

		const core::dimension2d<u32>& renderTargetSize = getCurrentRenderTargetSize();

		if (targetPos.X + sourceSize.Width > (s32)renderTargetSize.Width)
		{
			sourceSize.Width -= (targetPos.X + sourceSize.Width) - renderTargetSize.Width;
			if (sourceSize.Width <= 0)
				continue;
		}

		if (targetPos.Y<0)
		{
			sourceSize.Height += targetPos.Y;
			if (sourceSize.Height <= 0)
				continue;

			sourcePos.Y -= targetPos.Y;
			targetPos.Y = 0;
		}

		if (targetPos.Y + sourceSize.Height > (s32)renderTargetSize.Height)
		{
			sourceSize.Height -= (targetPos.Y + sourceSize.Height) - renderTargetSize.Height;
			if (sourceSize.Height <= 0)
				continue;
		}

		// ok, we've clipped everything.
		// now draw it.

		core::rect<f32> tcoords;
		tcoords.UpperLeftCorner.X = (((f32)sourcePos.X)) / texture->getOriginalSize().Width ;
		tcoords.UpperLeftCorner.Y = (((f32)sourcePos.Y)) / texture->getOriginalSize().Height;
		tcoords.LowerRightCorner.X = tcoords.UpperLeftCorner.X + ((f32)(sourceSize.Width) / texture->getOriginalSize().Width);
		tcoords.LowerRightCorner.Y = tcoords.UpperLeftCorner.Y + ((f32)(sourceSize.Height) / texture->getOriginalSize().Height);

		const core::rect<s32> poss(targetPos, sourceSize);

		vtx.push_back(S3DVertex((f32)poss.UpperLeftCorner.X, (f32)poss.UpperLeftCorner.Y, 0.0f,
				0.0f, 0.0f, 0.0f, color,
				tcoords.UpperLeftCorner.X, tcoords.UpperLeftCorner.Y));
		vtx.push_back(S3DVertex((f32)poss.LowerRightCorner.X, (f32)poss.UpperLeftCorner.Y, 0.0f,
				0.0f, 0.0f, 0.0f, color,
				tcoords.LowerRightCorner.X, tcoords.UpperLeftCorner.Y));
		vtx.push_back(S3DVertex((f32)poss.LowerRightCorner.X, (f32)poss.LowerRightCorner.Y, 0.0f,
				0.0f, 0.0f, 0.0f, color,
				tcoords.LowerRightCorner.X, tcoords.LowerRightCorner.Y));
		vtx.push_back(S3DVertex((f32)poss.UpperLeftCorner.X, (f32)poss.LowerRightCorner.Y, 0.0f,
				0.0f, 0.0f, 0.0f, color,
				tcoords.UpperLeftCorner.X, tcoords.LowerRightCorner.Y));

		const u32 curPos = vtx.size()-4;
		indices.push_back(0+curPos);
		indices.push_back(1+curPos);
		indices.push_back(2+curPos);

		indices.push_back(0+curPos);
		indices.push_back(2+curPos);
		indices.push_back(3+curPos);
	}

	if (vtx.size())
	{
		setVertexShader(EVT_STANDARD);

		pID3DDevice->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, vtx.size(), indices.size() / 3, indices.pointer(),
			D3DFMT_INDEX16,vtx.pointer(), sizeof(S3DVertex));
	}
}


//! draws a 2d image, using a color and the alpha channel of the texture if
//! desired. The image is drawn at pos and clipped against clipRect (if != 0).
void CD3D9Driver::draw2DImage(const video::ITexture* texture,
				const core::position2d<s32>& pos,
				const core::rect<s32>& sourceRect,
				const core::rect<s32>* clipRect, SColor color,
				bool useAlphaChannelOfTexture)
{
	if (!texture)
		return;

	if (!sourceRect.isValid())
		return;

	if (!setActiveTexture(0, const_cast<video::ITexture*>(texture)))
		return;

	core::position2d<s32> targetPos = pos;
	core::position2d<s32> sourcePos = sourceRect.UpperLeftCorner;
	// This needs to be signed as it may go negative.
	core::dimension2d<s32> sourceSize(sourceRect.getSize());

	if (clipRect)
	{
		if (targetPos.X < clipRect->UpperLeftCorner.X)
		{
			sourceSize.Width += targetPos.X - clipRect->UpperLeftCorner.X;
			if (sourceSize.Width <= 0)
				return;

			sourcePos.X -= targetPos.X - clipRect->UpperLeftCorner.X;
			targetPos.X = clipRect->UpperLeftCorner.X;
		}

		if (targetPos.X + (s32)sourceSize.Width > clipRect->LowerRightCorner.X)
		{
			sourceSize.Width -= (targetPos.X + sourceSize.Width) - clipRect->LowerRightCorner.X;
			if (sourceSize.Width <= 0)
				return;
		}

		if (targetPos.Y < clipRect->UpperLeftCorner.Y)
		{
			sourceSize.Height += targetPos.Y - clipRect->UpperLeftCorner.Y;
			if (sourceSize.Height <= 0)
				return;

			sourcePos.Y -= targetPos.Y - clipRect->UpperLeftCorner.Y;
			targetPos.Y = clipRect->UpperLeftCorner.Y;
		}

		if (targetPos.Y + (s32)sourceSize.Height > clipRect->LowerRightCorner.Y)
		{
			sourceSize.Height -= (targetPos.Y + sourceSize.Height) - clipRect->LowerRightCorner.Y;
			if (sourceSize.Height <= 0)
				return;
		}
	}

	// clip these coordinates

	if (targetPos.X<0)
	{
		sourceSize.Width += targetPos.X;
		if (sourceSize.Width <= 0)
			return;

		sourcePos.X -= targetPos.X;
		targetPos.X = 0;
	}

	const core::dimension2d<u32>& renderTargetSize = getCurrentRenderTargetSize();

	if (targetPos.X + sourceSize.Width > (s32)renderTargetSize.Width)
	{
		sourceSize.Width -= (targetPos.X + sourceSize.Width) - renderTargetSize.Width;
		if (sourceSize.Width <= 0)
			return;
	}

	if (targetPos.Y<0)
	{
		sourceSize.Height += targetPos.Y;
		if (sourceSize.Height <= 0)
			return;

		sourcePos.Y -= targetPos.Y;
		targetPos.Y = 0;
	}

	if (targetPos.Y + sourceSize.Height > (s32)renderTargetSize.Height)
	{
		sourceSize.Height -= (targetPos.Y + sourceSize.Height) - renderTargetSize.Height;
		if (sourceSize.Height <= 0)
			return;
	}

	// ok, we've clipped everything.
	// now draw it.

	core::rect<f32> tcoords;
	tcoords.UpperLeftCorner.X = (((f32)sourcePos.X)) / texture->getOriginalSize().Width ;
	tcoords.UpperLeftCorner.Y = (((f32)sourcePos.Y)) / texture->getOriginalSize().Height;
	tcoords.LowerRightCorner.X = tcoords.UpperLeftCorner.X + ((f32)(sourceSize.Width) / texture->getOriginalSize().Width);
	tcoords.LowerRightCorner.Y = tcoords.UpperLeftCorner.Y + ((f32)(sourceSize.Height) / texture->getOriginalSize().Height);

	const core::rect<s32> poss(targetPos, sourceSize);

	setRenderStates2DMode(color.getAlpha()<255, true, useAlphaChannelOfTexture);

	S3DVertex vtx[4];
	vtx[0] = S3DVertex((f32)poss.UpperLeftCorner.X, (f32)poss.UpperLeftCorner.Y, 0.0f,
			0.0f, 0.0f, 0.0f, color,
			tcoords.UpperLeftCorner.X, tcoords.UpperLeftCorner.Y);
	vtx[1] = S3DVertex((f32)poss.LowerRightCorner.X, (f32)poss.UpperLeftCorner.Y, 0.0f,
			0.0f, 0.0f, 0.0f, color,
			tcoords.LowerRightCorner.X, tcoords.UpperLeftCorner.Y);
	vtx[2] = S3DVertex((f32)poss.LowerRightCorner.X, (f32)poss.LowerRightCorner.Y, 0.0f,
			0.0f, 0.0f, 0.0f, color,
			tcoords.LowerRightCorner.X, tcoords.LowerRightCorner.Y);
	vtx[3] = S3DVertex((f32)poss.UpperLeftCorner.X, (f32)poss.LowerRightCorner.Y, 0.0f,
			0.0f, 0.0f, 0.0f, color,
			tcoords.UpperLeftCorner.X, tcoords.LowerRightCorner.Y);

	s16 indices[6] = {0,1,2,0,2,3};

	setVertexShader(EVT_STANDARD);

	pID3DDevice->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 4, 2, &indices[0],
		D3DFMT_INDEX16,&vtx[0],	sizeof(S3DVertex));
}


//!Draws a 2d rectangle with a gradient.
void CD3D9Driver::draw2DRectangle(const core::rect<s32>& position,
	SColor colorLeftUp, SColor colorRightUp, SColor colorLeftDown, SColor colorRightDown,
	const core::rect<s32>* clip)
{
	core::rect<s32> pos(position);

	if (clip)
		pos.clipAgainst(*clip);

	if (!pos.isValid())
		return;

	S3DVertex vtx[4];
	vtx[0] = S3DVertex((f32)pos.UpperLeftCorner.X, (f32)pos.UpperLeftCorner.Y, 0.0f,
			0.0f, 0.0f, 0.0f, colorLeftUp, 0.0f, 0.0f);
	vtx[1] = S3DVertex((f32)pos.LowerRightCorner.X, (f32)pos.UpperLeftCorner.Y, 0.0f,
			0.0f, 0.0f, 0.0f, colorRightUp, 0.0f, 1.0f);
	vtx[2] = S3DVertex((f32)pos.LowerRightCorner.X, (f32)pos.LowerRightCorner.Y, 0.0f,
			0.0f, 0.0f, 0.0f, colorRightDown, 1.0f, 0.0f);
	vtx[3] = S3DVertex((f32)pos.UpperLeftCorner.X, (f32)pos.LowerRightCorner.Y, 0.0f,
			0.0f, 0.0f, 0.0f, colorLeftDown, 1.0f, 1.0f);

	s16 indices[6] = {0,1,2,0,2,3};

	setRenderStates2DMode(
		colorLeftUp.getAlpha() < 255 ||
		colorRightUp.getAlpha() < 255 ||
		colorLeftDown.getAlpha() < 255 ||
		colorRightDown.getAlpha() < 255, false, false);

	setActiveTexture(0,0);

	setVertexShader(EVT_STANDARD);

	pID3DDevice->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 4, 2, &indices[0],
		D3DFMT_INDEX16, &vtx[0], sizeof(S3DVertex));
}


//! Draws a 2d line.
void CD3D9Driver::draw2DLine(const core::position2d<s32>& start,
				const core::position2d<s32>& end,
				SColor color)
{
	if (start==end)
		drawPixel(start.X, start.Y, color);
	else
	{
		// thanks to Vash TheStampede who sent in his implementation
		S3DVertex vtx[2];
		vtx[0] = S3DVertex((f32)start.X+0.375f, (f32)start.Y+0.375f, 0.0f,
							0.0f, 0.0f, 0.0f, // normal
							color, 0.0f, 0.0f); // texture

		vtx[1] = S3DVertex((f32)end.X+0.375f, (f32)end.Y+0.375f, 0.0f,
							0.0f, 0.0f, 0.0f,
							color, 0.0f, 0.0f);

		setRenderStates2DMode(color.getAlpha() < 255, false, false);
		setActiveTexture(0,0);

		setVertexShader(EVT_STANDARD);

		pID3DDevice->DrawPrimitiveUP(D3DPT_LINELIST, 1,
						&vtx[0], sizeof(S3DVertex) );
	}
}


//! Draws a pixel
void CD3D9Driver::drawPixel(u32 x, u32 y, const SColor & color)
{
	const core::dimension2d<u32>& renderTargetSize = getCurrentRenderTargetSize();
	if(x > (u32)renderTargetSize.Width || y > (u32)renderTargetSize.Height)
		return;

	setRenderStates2DMode(color.getAlpha() < 255, false, false);
	setActiveTexture(0,0);

	setVertexShader(EVT_STANDARD);

	S3DVertex vertex((f32)x+0.375f, (f32)y+0.375f, 0.f, 0.f, 0.f, 0.f, color, 0.f, 0.f);

	pID3DDevice->DrawPrimitiveUP(D3DPT_POINTLIST, 1, &vertex, sizeof(vertex));
}


//! sets right vertex shader
void CD3D9Driver::setVertexShader(E_VERTEX_TYPE newType)
{
	if (newType != LastVertexType)
	{
		LastVertexType = newType;
		HRESULT hr = 0;

		switch(newType)
		{
		case EVT_STANDARD:
			hr = pID3DDevice->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1);
			break;
		case EVT_2TCOORDS:
			hr = pID3DDevice->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX2);
			break;
		case EVT_TANGENTS:
			hr = pID3DDevice->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX3 |
				D3DFVF_TEXCOORDSIZE2(0) | // real texture coord
				D3DFVF_TEXCOORDSIZE3(1) | // misuse texture coord 2 for tangent
				D3DFVF_TEXCOORDSIZE3(2) // misuse texture coord 3 for binormal
				);
			break;
		}

		if (FAILED(hr))
		{
			os::Printer::log("Could not set vertex Shader.", ELL_ERROR);
			return;
		}
	}
}


//! sets the needed renderstates
bool CD3D9Driver::setRenderStates3DMode()
{
	if (!pID3DDevice)
		return false;

	if (CurrentRenderMode != ERM_3D)
	{
		// switch back the matrices
		pID3DDevice->SetTransform(D3DTS_VIEW, (D3DMATRIX*)((void*)&Matrices[ETS_VIEW]));
		pID3DDevice->SetTransform(D3DTS_WORLD, (D3DMATRIX*)((void*)&Matrices[ETS_WORLD]));
		pID3DDevice->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)((void*)&Matrices[ETS_PROJECTION]));

		pID3DDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
		pID3DDevice->SetRenderState(D3DRS_CLIPPING, TRUE);

		ResetRenderStates = true;
	}

	if (ResetRenderStates || LastMaterial != Material)
	{
		// unset old material

		if (CurrentRenderMode == ERM_3D &&
			LastMaterial.MaterialType != Material.MaterialType &&
			LastMaterial.MaterialType >= 0 && LastMaterial.MaterialType < (s32)MaterialRenderers.size())
			MaterialRenderers[LastMaterial.MaterialType].Renderer->OnUnsetMaterial();

		// set new material.

		if (Material.MaterialType >= 0 && Material.MaterialType < (s32)MaterialRenderers.size())
			MaterialRenderers[Material.MaterialType].Renderer->OnSetMaterial(
				Material, LastMaterial, ResetRenderStates, this);
	}

	bool shaderOK = true;
	if (Material.MaterialType >= 0 && Material.MaterialType < (s32)MaterialRenderers.size())
		shaderOK = MaterialRenderers[Material.MaterialType].Renderer->OnRender(this, LastVertexType);

	LastMaterial = Material;

	ResetRenderStates = false;

	CurrentRenderMode = ERM_3D;

	return shaderOK;
}


//! Map Irrlicht texture wrap mode to native values
D3DTEXTUREADDRESS CD3D9Driver::getTextureWrapMode(const u8 clamp)
{
	switch (clamp)
	{
		case ETC_REPEAT:
			if (Caps.TextureAddressCaps & D3DPTADDRESSCAPS_WRAP)
				return D3DTADDRESS_WRAP;
		case ETC_CLAMP:
		case ETC_CLAMP_TO_EDGE:
			if (Caps.TextureAddressCaps & D3DPTADDRESSCAPS_CLAMP)
				return D3DTADDRESS_CLAMP;
		case ETC_MIRROR:
			if (Caps.TextureAddressCaps & D3DPTADDRESSCAPS_MIRROR)
				return D3DTADDRESS_MIRROR;
		case ETC_CLAMP_TO_BORDER:
			if (Caps.TextureAddressCaps & D3DPTADDRESSCAPS_BORDER)
				return D3DTADDRESS_BORDER;
			else
				return D3DTADDRESS_CLAMP;
		case ETC_MIRROR_CLAMP:
		case ETC_MIRROR_CLAMP_TO_EDGE:
		case ETC_MIRROR_CLAMP_TO_BORDER:
			if (Caps.TextureAddressCaps & D3DPTADDRESSCAPS_MIRRORONCE)
				return D3DTADDRESS_MIRRORONCE;
			else
				return D3DTADDRESS_CLAMP;
		default:
			return D3DTADDRESS_WRAP;
	}
}


//! Can be called by an IMaterialRenderer to make its work easier.
void CD3D9Driver::setBasicRenderStates(const SMaterial& material, const SMaterial& lastmaterial,
	bool resetAllRenderstates)
{
	// This needs only to be updated onresets
	if (Params.HandleSRGB && resetAllRenderstates)
		pID3DDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, TRUE);

	if (resetAllRenderstates ||
		lastmaterial.AmbientColor != material.AmbientColor ||
		lastmaterial.DiffuseColor != material.DiffuseColor ||
		lastmaterial.SpecularColor != material.SpecularColor ||
		lastmaterial.EmissiveColor != material.EmissiveColor ||
		lastmaterial.Shininess != material.Shininess)
	{
		D3DMATERIAL9 mat;
		mat.Diffuse = colorToD3D(material.DiffuseColor);
		mat.Ambient = colorToD3D(material.AmbientColor);
		mat.Specular = colorToD3D(material.SpecularColor);
		mat.Emissive = colorToD3D(material.EmissiveColor);
		mat.Power = material.Shininess;
		pID3DDevice->SetMaterial(&mat);
	}

	if (lastmaterial.ColorMaterial != material.ColorMaterial)
	{
		pID3DDevice->SetRenderState(D3DRS_COLORVERTEX, (material.ColorMaterial != ECM_NONE));
		pID3DDevice->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE,
			((material.ColorMaterial == ECM_DIFFUSE)||
			(material.ColorMaterial == ECM_DIFFUSE_AND_AMBIENT))?D3DMCS_COLOR1:D3DMCS_MATERIAL);
		pID3DDevice->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE,
			((material.ColorMaterial == ECM_AMBIENT)||
			(material.ColorMaterial == ECM_DIFFUSE_AND_AMBIENT))?D3DMCS_COLOR1:D3DMCS_MATERIAL);
		pID3DDevice->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE,
			(material.ColorMaterial == ECM_EMISSIVE)?D3DMCS_COLOR1:D3DMCS_MATERIAL);
		pID3DDevice->SetRenderState(D3DRS_SPECULARMATERIALSOURCE,
			(material.ColorMaterial == ECM_SPECULAR)?D3DMCS_COLOR1:D3DMCS_MATERIAL);
	}

	// fillmode
	if (resetAllRenderstates || lastmaterial.Wireframe != material.Wireframe || lastmaterial.PointCloud != material.PointCloud)
	{
		if (material.Wireframe)
			pID3DDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
		else
		if (material.PointCloud)
			pID3DDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_POINT);
		else
			pID3DDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	}

	// shademode

	if (resetAllRenderstates || lastmaterial.GouraudShading != material.GouraudShading)
	{
		if (material.GouraudShading)
			pID3DDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
		else
			pID3DDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);
	}

	// lighting

	if (resetAllRenderstates || lastmaterial.Lighting != material.Lighting)
	{
		if (material.Lighting)
			pID3DDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
		else
			pID3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	}

	// zbuffer

	if (resetAllRenderstates || lastmaterial.ZBuffer != material.ZBuffer)
	{
		switch (material.ZBuffer)
		{
		case ECFN_NEVER:
			pID3DDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
			break;
		case ECFN_LESSEQUAL:
			pID3DDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
			pID3DDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
			break;
		case ECFN_EQUAL:
			pID3DDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
			pID3DDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_EQUAL);
			break;
		case ECFN_LESS:
			pID3DDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
			pID3DDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);
			break;
		case ECFN_NOTEQUAL:
			pID3DDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
			pID3DDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_NOTEQUAL);
			break;
		case ECFN_GREATEREQUAL:
			pID3DDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
			pID3DDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATEREQUAL);
			break;
		case ECFN_GREATER:
			pID3DDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
			pID3DDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_GREATER);
			break;
		case ECFN_ALWAYS:
			pID3DDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
			pID3DDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
			break;
		}
	}

	// zwrite
//	if (resetAllRenderstates || (lastmaterial.ZWriteEnable != material.ZWriteEnable))
	{
		if ( material.ZWriteEnable && (AllowZWriteOnTransparent || !material.isTransparent()))
			pID3DDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE);
		else
			pID3DDevice->SetRenderState( D3DRS_ZWRITEENABLE, FALSE);
	}

	// back face culling

	if (resetAllRenderstates || (lastmaterial.FrontfaceCulling != material.FrontfaceCulling) || (lastmaterial.BackfaceCulling != material.BackfaceCulling))
	{
//		if (material.FrontfaceCulling && material.BackfaceCulling)
//			pID3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW|D3DCULL_CCW);
//		else
		if (material.FrontfaceCulling)
			pID3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
		else
		if (material.BackfaceCulling)
			pID3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
		else
			pID3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	}

	// fog
	if (resetAllRenderstates || lastmaterial.FogEnable != material.FogEnable)
	{
		pID3DDevice->SetRenderState(D3DRS_FOGENABLE, material.FogEnable);
	}

	// specular highlights
	if (resetAllRenderstates || !core::equals(lastmaterial.Shininess,material.Shininess))
	{
		const bool enable = (material.Shininess!=0.0f);
		pID3DDevice->SetRenderState(D3DRS_SPECULARENABLE, enable);
		pID3DDevice->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_MATERIAL);
	}

	// normalization
	if (resetAllRenderstates || lastmaterial.NormalizeNormals != material.NormalizeNormals)
	{
		pID3DDevice->SetRenderState(D3DRS_NORMALIZENORMALS, material.NormalizeNormals);
	}

	// Color Mask
	if (queryFeature(EVDF_COLOR_MASK) &&
		(resetAllRenderstates || lastmaterial.ColorMask != material.ColorMask))
	{
		const DWORD flag =
			((material.ColorMask & ECP_RED)?D3DCOLORWRITEENABLE_RED:0) |
			((material.ColorMask & ECP_GREEN)?D3DCOLORWRITEENABLE_GREEN:0) |
			((material.ColorMask & ECP_BLUE)?D3DCOLORWRITEENABLE_BLUE:0) |
			((material.ColorMask & ECP_ALPHA)?D3DCOLORWRITEENABLE_ALPHA:0);
		pID3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE, flag);
	}

	if (queryFeature(EVDF_BLEND_OPERATIONS) &&
		(resetAllRenderstates|| lastmaterial.BlendOperation != material.BlendOperation))
	{
		if (material.BlendOperation==EBO_NONE)
			pID3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		else
		{
			pID3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			switch (material.BlendOperation)
			{
			case EBO_SUBTRACT:
				pID3DDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_SUBTRACT);
				break;
			case EBO_REVSUBTRACT:
				pID3DDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_REVSUBTRACT);
				break;
			case EBO_MIN:
			case EBO_MIN_FACTOR:
			case EBO_MIN_ALPHA:
				pID3DDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_MIN);
				break;
			case EBO_MAX:
			case EBO_MAX_FACTOR:
			case EBO_MAX_ALPHA:
				pID3DDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_MAX);
				break;
			default:
				pID3DDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
				break;
			}
		}
	}

	// Polygon offset
	if (queryFeature(EVDF_POLYGON_OFFSET) && (resetAllRenderstates ||
		lastmaterial.PolygonOffsetDirection != material.PolygonOffsetDirection ||
		lastmaterial.PolygonOffsetFactor != material.PolygonOffsetFactor))
	{
		if (material.PolygonOffsetFactor)
		{
			if (material.PolygonOffsetDirection==EPO_BACK)
			{
				pID3DDevice->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, F2DW(1.f));
				pID3DDevice->SetRenderState(D3DRS_DEPTHBIAS, F2DW((FLOAT)material.PolygonOffsetFactor));
			}
			else
			{
				pID3DDevice->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, F2DW(-1.f));
				pID3DDevice->SetRenderState(D3DRS_DEPTHBIAS, F2DW((FLOAT)-material.PolygonOffsetFactor));
			}
		}
		else
		{
			pID3DDevice->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, 0);
			pID3DDevice->SetRenderState(D3DRS_DEPTHBIAS, 0);
		}
	}

	// Anti Aliasing
	if (resetAllRenderstates || lastmaterial.AntiAliasing != material.AntiAliasing)
	{
		if (AlphaToCoverageSupport && (material.AntiAliasing & EAAM_ALPHA_TO_COVERAGE))
		{
			if (VendorID==0x10DE)//NVidia
				pID3DDevice->SetRenderState(D3DRS_ADAPTIVETESS_Y, MAKEFOURCC('A','T','O','C'));
			// SSAA could give better results on NVidia cards
			else if (VendorID==0x1002)//ATI
				pID3DDevice->SetRenderState(D3DRS_POINTSIZE, MAKEFOURCC('A','2','M','1'));
		}
		else if (AlphaToCoverageSupport && (lastmaterial.AntiAliasing & EAAM_ALPHA_TO_COVERAGE))
		{
			if (VendorID==0x10DE)
				pID3DDevice->SetRenderState(D3DRS_ADAPTIVETESS_Y, D3DFMT_UNKNOWN);
			else if (VendorID==0x1002)
				pID3DDevice->SetRenderState(D3DRS_POINTSIZE, MAKEFOURCC('A','2','M','0'));
		}

		// enable antialiasing
		if (Params.AntiAlias)
		{
			if (material.AntiAliasing & (EAAM_SIMPLE|EAAM_QUALITY))
				pID3DDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
			else if (lastmaterial.AntiAliasing & (EAAM_SIMPLE|EAAM_QUALITY))
				pID3DDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
			if (material.AntiAliasing & (EAAM_LINE_SMOOTH))
				pID3DDevice->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
			else if (lastmaterial.AntiAliasing & (EAAM_LINE_SMOOTH))
				pID3DDevice->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
		}
	}

	// thickness
	if (resetAllRenderstates || lastmaterial.Thickness != material.Thickness)
	{
		pID3DDevice->SetRenderState(D3DRS_POINTSIZE, F2DW(material.Thickness));
	}

	// texture address mode
	for (u32 st=0; st<MaxTextureUnits; ++st)
	{
		if (resetAllRenderstates && Params.HandleSRGB)
			pID3DDevice->SetSamplerState(st, D3DSAMP_SRGBTEXTURE, TRUE);

		if (resetAllRenderstates || lastmaterial.TextureLayer[st].LODBias != material.TextureLayer[st].LODBias)
		{
			const float tmp = material.TextureLayer[st].LODBias * 0.125f;
			pID3DDevice->SetSamplerState(st, D3DSAMP_MIPMAPLODBIAS, F2DW(tmp));
		}

		if (resetAllRenderstates || lastmaterial.TextureLayer[st].TextureWrapU != material.TextureLayer[st].TextureWrapU)
			pID3DDevice->SetSamplerState(st, D3DSAMP_ADDRESSU, getTextureWrapMode(material.TextureLayer[st].TextureWrapU));
		// If separate UV not supported reuse U for V
		if (!(Caps.TextureAddressCaps & D3DPTADDRESSCAPS_INDEPENDENTUV))
			pID3DDevice->SetSamplerState(st, D3DSAMP_ADDRESSV, getTextureWrapMode(material.TextureLayer[st].TextureWrapU));
		else if (resetAllRenderstates || lastmaterial.TextureLayer[st].TextureWrapV != material.TextureLayer[st].TextureWrapV)
			pID3DDevice->SetSamplerState(st, D3DSAMP_ADDRESSV, getTextureWrapMode(material.TextureLayer[st].TextureWrapV));

		// Bilinear, trilinear, and anisotropic filter
		if (resetAllRenderstates ||
			lastmaterial.TextureLayer[st].BilinearFilter != material.TextureLayer[st].BilinearFilter ||
			lastmaterial.TextureLayer[st].TrilinearFilter != material.TextureLayer[st].TrilinearFilter ||
			lastmaterial.TextureLayer[st].AnisotropicFilter != material.TextureLayer[st].AnisotropicFilter ||
			lastmaterial.UseMipMaps != material.UseMipMaps)
		{
			if (material.TextureLayer[st].BilinearFilter || material.TextureLayer[st].TrilinearFilter || material.TextureLayer[st].AnisotropicFilter)
			{
				D3DTEXTUREFILTERTYPE tftMag = ((Caps.TextureFilterCaps & D3DPTFILTERCAPS_MAGFANISOTROPIC) &&
						material.TextureLayer[st].AnisotropicFilter) ? D3DTEXF_ANISOTROPIC : D3DTEXF_LINEAR;
				D3DTEXTUREFILTERTYPE tftMin = ((Caps.TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC) &&
						material.TextureLayer[st].AnisotropicFilter) ? D3DTEXF_ANISOTROPIC : D3DTEXF_LINEAR;
				D3DTEXTUREFILTERTYPE tftMip = material.UseMipMaps? (material.TextureLayer[st].TrilinearFilter ? D3DTEXF_LINEAR : D3DTEXF_POINT) : D3DTEXF_NONE;

				if (tftMag==D3DTEXF_ANISOTROPIC || tftMin == D3DTEXF_ANISOTROPIC)
					pID3DDevice->SetSamplerState(st, D3DSAMP_MAXANISOTROPY, core::min_((DWORD)material.TextureLayer[st].AnisotropicFilter, Caps.MaxAnisotropy));
				pID3DDevice->SetSamplerState(st, D3DSAMP_MAGFILTER, tftMag);
				pID3DDevice->SetSamplerState(st, D3DSAMP_MINFILTER, tftMin);
				pID3DDevice->SetSamplerState(st, D3DSAMP_MIPFILTER, tftMip);
			}
			else
			{
				pID3DDevice->SetSamplerState(st, D3DSAMP_MINFILTER, D3DTEXF_POINT);
				pID3DDevice->SetSamplerState(st, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
				pID3DDevice->SetSamplerState(st, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
			}
		}
	}
}


//! sets the needed renderstates
void CD3D9Driver::setRenderStatesStencilShadowMode(bool zfail, u32 debugDataVisible)
{
	if ((CurrentRenderMode != ERM_SHADOW_VOLUME_ZFAIL &&
		CurrentRenderMode != ERM_SHADOW_VOLUME_ZPASS) ||
		Transformation3DChanged)
	{
		// unset last 3d material
		if (CurrentRenderMode == ERM_3D &&
			static_cast<u32>(Material.MaterialType) < MaterialRenderers.size())
		{
			MaterialRenderers[Material.MaterialType].Renderer->OnUnsetMaterial();
			ResetRenderStates = true;
		}
		// switch back the matrices
		pID3DDevice->SetTransform(D3DTS_VIEW, (D3DMATRIX*)((void*)&Matrices[ETS_VIEW]));
		pID3DDevice->SetTransform(D3DTS_WORLD, (D3DMATRIX*)((void*)&Matrices[ETS_WORLD]));
		pID3DDevice->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)((void*)&Matrices[ETS_PROJECTION]));

		Transformation3DChanged = false;

		setActiveTexture(0,0);
		setActiveTexture(1,0);
		setActiveTexture(2,0);
		setActiveTexture(3,0);

		pID3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_DISABLE);

		pID3DDevice->SetFVF(D3DFVF_XYZ);
		LastVertexType = (video::E_VERTEX_TYPE)(-1);

		pID3DDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
		pID3DDevice->SetRenderState(D3DRS_STENCILENABLE, TRUE);
		pID3DDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_FLAT);
		//pID3DDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);
		//pID3DDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

		pID3DDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
		pID3DDevice->SetRenderState(D3DRS_STENCILREF, 0x0);
		pID3DDevice->SetRenderState(D3DRS_STENCILMASK, 0xffffffff);
		pID3DDevice->SetRenderState(D3DRS_STENCILWRITEMASK, 0xffffffff);

		pID3DDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
		pID3DDevice->SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ZERO );
		pID3DDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );

		pID3DDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
		pID3DDevice->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESS);

		//if (!(debugDataVisible & (scene::EDS_SKELETON|scene::EDS_MESH_WIRE_OVERLAY)))
		//	pID3DDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
		if ((debugDataVisible & scene::EDS_MESH_WIRE_OVERLAY))
			pID3DDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
	}

	if (CurrentRenderMode != ERM_SHADOW_VOLUME_ZPASS && !zfail)
	{
		// USE THE ZPASS METHOD
		pID3DDevice->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
		pID3DDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
		//pID3DDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_INCR);	// does not matter, will be set later
	}
	else
	if (CurrentRenderMode != ERM_SHADOW_VOLUME_ZFAIL && zfail)
	{
		// USE THE ZFAIL METHOD
		pID3DDevice->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
		//pID3DDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_INCR);	// does not matter, will be set later
		pID3DDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
	}

	CurrentRenderMode = zfail ? ERM_SHADOW_VOLUME_ZFAIL : ERM_SHADOW_VOLUME_ZPASS;
}


//! sets the needed renderstates
void CD3D9Driver::setRenderStatesStencilFillMode(bool alpha)
{
	if (CurrentRenderMode != ERM_STENCIL_FILL || Transformation3DChanged)
	{
		core::matrix4 mat;
		pID3DDevice->SetTransform(D3DTS_VIEW, &UnitMatrixD3D9);
		pID3DDevice->SetTransform(D3DTS_WORLD, &UnitMatrixD3D9);
		pID3DDevice->SetTransform(D3DTS_PROJECTION, &UnitMatrixD3D9);

		pID3DDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
		pID3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
		pID3DDevice->SetRenderState(D3DRS_FOGENABLE, FALSE);

		pID3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);

		pID3DDevice->SetRenderState(D3DRS_STENCILREF, 0x1);
		pID3DDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_LESSEQUAL);
		//pID3DDevice->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_GREATEREQUAL);
		pID3DDevice->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
		pID3DDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
		pID3DDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
		pID3DDevice->SetRenderState(D3DRS_STENCILMASK, 0xffffffff);
		pID3DDevice->SetRenderState(D3DRS_STENCILWRITEMASK, 0xffffffff);

		pID3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

		Transformation3DChanged = false;

		pID3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		pID3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		pID3DDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		pID3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		pID3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
		if (alpha)
		{
			pID3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
			pID3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			pID3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		}
		else
		{
			pID3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		}
	}

	CurrentRenderMode = ERM_STENCIL_FILL;
}


//! Enable the 2d override material
void CD3D9Driver::enableMaterial2D(bool enable)
{
	if (!enable)
		CurrentRenderMode = ERM_NONE;
	CNullDriver::enableMaterial2D(enable);
}


//! sets the needed renderstates
void CD3D9Driver::setRenderStates2DMode(bool alpha, bool texture, bool alphaChannel)
{
	if (!pID3DDevice)
		return;

	if (CurrentRenderMode != ERM_2D || Transformation3DChanged)
	{
		// unset last 3d material
		if (CurrentRenderMode == ERM_3D)
		{
			if (static_cast<u32>(LastMaterial.MaterialType) < MaterialRenderers.size())
				MaterialRenderers[LastMaterial.MaterialType].Renderer->OnUnsetMaterial();
		}
		if (!OverrideMaterial2DEnabled)
		{
			setBasicRenderStates(InitMaterial2D, LastMaterial, true);
			LastMaterial=InitMaterial2D;

			// fix everything that is wrongly set by InitMaterial2D default
			pID3DDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);

			pID3DDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
		}
		core::matrix4 m;
// this fixes some problems with pixel exact rendering, but also breaks nice texturing
// moreover, it would have to be tested in each call, as the texture flag can change each time
//		if (!texture)
//			m.setTranslation(core::vector3df(0.5f,0.5f,0));
		pID3DDevice->SetTransform(D3DTS_WORLD, (D3DMATRIX*)((void*)m.pointer()));

		// adjust the view such that pixel center aligns with texels
		// Otherwise, subpixel artifacts will occur
		m.setTranslation(core::vector3df(-0.5f,-0.5f,0));
		pID3DDevice->SetTransform(D3DTS_VIEW, (D3DMATRIX*)((void*)m.pointer()));

		const core::dimension2d<u32>& renderTargetSize = getCurrentRenderTargetSize();
		m.buildProjectionMatrixOrthoLH(f32(renderTargetSize.Width), f32(-(s32)(renderTargetSize.Height)), -1.0, 1.0);
		m.setTranslation(core::vector3df(-1,1,0));
		pID3DDevice->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)((void*)m.pointer()));

		// 2d elements are clipped in software
		pID3DDevice->SetRenderState(D3DRS_CLIPPING, FALSE);

		Transformation3DChanged = false;
	}
	if (OverrideMaterial2DEnabled)
	{
		OverrideMaterial2D.Lighting=false;
		setBasicRenderStates(OverrideMaterial2D, LastMaterial, false);
		LastMaterial = OverrideMaterial2D;
	}

	// no alphaChannel without texture
	alphaChannel &= texture;

	if (alpha || alphaChannel)
	{
		pID3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		pID3DDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		pID3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	}
	else
		pID3DDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	pID3DDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	pID3DDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	pID3DDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
	if (texture)
	{
		setTransform(ETS_TEXTURE_0, core::IdentityMatrix);
		// Due to the transformation change, the previous line would call a reset each frame
		// but we can safely reset the variable as it was false before
		Transformation3DChanged=false;
	}
	if (alphaChannel)
	{
		pID3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);

		if (alpha)
		{
			pID3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
			pID3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		}
		else
		{
			pID3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
		}
	}
	else
	{
		pID3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		if (alpha)
		{
			pID3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2);
		}
		else
		{
			pID3DDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			pID3DDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		}
	}

	CurrentRenderMode = ERM_2D;
}


//! deletes all dynamic lights there are
void CD3D9Driver::deleteAllDynamicLights()
{
	for (s32 i=0; i<LastSetLight+1; ++i)
		pID3DDevice->LightEnable(i, false);

	LastSetLight = -1;

	CNullDriver::deleteAllDynamicLights();
}


//! adds a dynamic light
s32 CD3D9Driver::addDynamicLight(const SLight& dl)
{
	CNullDriver::addDynamicLight(dl);

	D3DLIGHT9 light;

	switch (dl.Type)
	{
	case ELT_POINT:
		light.Type = D3DLIGHT_POINT;
	break;
	case ELT_SPOT:
		light.Type = D3DLIGHT_SPOT;
	break;
	case ELT_DIRECTIONAL:
		light.Type = D3DLIGHT_DIRECTIONAL;
	break;
	}

	light.Position = *(D3DVECTOR*)((void*)(&dl.Position));
	light.Direction = *(D3DVECTOR*)((void*)(&dl.Direction));

	light.Range = core::min_(dl.Radius, MaxLightDistance);
	light.Falloff = dl.Falloff;

	light.Diffuse = *(D3DCOLORVALUE*)((void*)(&dl.DiffuseColor));
	light.Specular = *(D3DCOLORVALUE*)((void*)(&dl.SpecularColor));
	light.Ambient = *(D3DCOLORVALUE*)((void*)(&dl.AmbientColor));

	light.Attenuation0 = dl.Attenuation.X;
	light.Attenuation1 = dl.Attenuation.Y;
	light.Attenuation2 = dl.Attenuation.Z;

	light.Theta = dl.InnerCone * 2.0f * core::DEGTORAD;
	light.Phi = dl.OuterCone * 2.0f * core::DEGTORAD;

	++LastSetLight;

	if(D3D_OK == pID3DDevice->SetLight(LastSetLight, &light))
	{
		// I don't care if this succeeds
		(void)pID3DDevice->LightEnable(LastSetLight, true);
		return LastSetLight;
	}

	return -1;
}

//! Turns a dynamic light on or off
//! \param lightIndex: the index returned by addDynamicLight
//! \param turnOn: true to turn the light on, false to turn it off
void CD3D9Driver::turnLightOn(s32 lightIndex, bool turnOn)
{
	if(lightIndex < 0 || lightIndex > LastSetLight)
		return;

	(void)pID3DDevice->LightEnable(lightIndex, turnOn);
}


//! returns the maximal amount of dynamic lights the device can handle
u32 CD3D9Driver::getMaximalDynamicLightAmount() const
{
	return Caps.MaxActiveLights;
}


//! Sets the dynamic ambient light color. The default color is
//! (0,0,0,0) which means it is dark.
//! \param color: New color of the ambient light.
void CD3D9Driver::setAmbientLight(const SColorf& color)
{
	if (!pID3DDevice)
		return;

	AmbientLight = color;
	D3DCOLOR col = color.toSColor().color;
	pID3DDevice->SetRenderState(D3DRS_AMBIENT, col);
}


//! \return Returns the name of the video driver. Example: In case of the DIRECT3D9
//! driver, it would return "Direct3D9.0".
const wchar_t* CD3D9Driver::getName() const
{
	return L"Direct3D 9.0";
}


//! Draws a shadow volume into the stencil buffer. To draw a stencil shadow, do
//! this: First, draw all geometry. Then use this method, to draw the shadow
//! volume. Then, use IVideoDriver::drawStencilShadow() to visualize the shadow.
void CD3D9Driver::drawStencilShadowVolume(const core::array<core::vector3df>& triangles, bool zfail, u32 debugDataVisible)
{
	if (!Params.Stencilbuffer)
		return;

	setRenderStatesStencilShadowMode(zfail, debugDataVisible);

	const u32 count = triangles.size();
	if (!count)
		return;

	if (!zfail)
	{
		// ZPASS Method

		// Draw front-side of shadow volume in stencil only
		pID3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
		pID3DDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_INCR);
		pID3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, count / 3, triangles.const_pointer(), sizeof(core::vector3df));

		// Now reverse cull order so front sides of shadow volume are written.
		pID3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
		pID3DDevice->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_DECR);
		pID3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, count / 3, triangles.const_pointer(), sizeof(core::vector3df));
	}
	else
	{
		// ZFAIL Method

		// Draw front-side of shadow volume in stencil only
		pID3DDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
		pID3DDevice->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_INCR);
		pID3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, count / 3, triangles.const_pointer(), sizeof(core::vector3df));

		// Now reverse cull order so front sides of shadow volume are written.
		pID3DDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW);
		pID3DDevice->SetRenderState( D3DRS_STENCILZFAIL, D3DSTENCILOP_DECR);
		pID3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLELIST, count / 3, triangles.const_pointer(), sizeof(core::vector3df));
	}
}


//! Fills the stencil shadow with color. After the shadow volume has been drawn
//! into the stencil buffer using IVideoDriver::drawStencilShadowVolume(), use this
//! to draw the color of the shadow.
void CD3D9Driver::drawStencilShadow(bool clearStencilBuffer, video::SColor leftUpEdge,
			video::SColor rightUpEdge, video::SColor leftDownEdge, video::SColor rightDownEdge)
{
	if (!Params.Stencilbuffer)
		return;

	S3DVertex vtx[4];
	vtx[0] = S3DVertex(1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, leftUpEdge, 0.0f, 0.0f);
	vtx[1] = S3DVertex(1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, rightUpEdge, 0.0f, 1.0f);
	vtx[2] = S3DVertex(-1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, leftDownEdge, 1.0f, 0.0f);
	vtx[3] = S3DVertex(-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, rightDownEdge, 1.0f, 1.0f);

	s16 indices[6] = {0,1,2,1,3,2};

	setRenderStatesStencilFillMode(
		leftUpEdge.getAlpha() < 255 ||
		rightUpEdge.getAlpha() < 255 ||
		leftDownEdge.getAlpha() < 255 ||
		rightDownEdge.getAlpha() < 255);

	setActiveTexture(0,0);

	setVertexShader(EVT_STANDARD);

	pID3DDevice->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 4, 2, &indices[0],
		D3DFMT_INDEX16, &vtx[0], sizeof(S3DVertex));

	if (clearStencilBuffer)
		pID3DDevice->Clear( 0, NULL, D3DCLEAR_STENCIL,0, 1.0, 0);
}


//! Returns the maximum amount of primitives (mostly vertices) which
//! the device is able to render with one drawIndexedTriangleList
//! call.
u32 CD3D9Driver::getMaximalPrimitiveCount() const
{
	return Caps.MaxPrimitiveCount;
}


//! Sets the fog mode.
void CD3D9Driver::setFog(SColor color, E_FOG_TYPE fogType, f32 start,
	f32 end, f32 density, bool pixelFog, bool rangeFog)
{
	CNullDriver::setFog(color, fogType, start, end, density, pixelFog, rangeFog);

	if (!pID3DDevice)
		return;

	pID3DDevice->SetRenderState(D3DRS_FOGCOLOR, color.color);

	pID3DDevice->SetRenderState(
		pixelFog ? D3DRS_FOGTABLEMODE : D3DRS_FOGVERTEXMODE,
		(fogType==EFT_FOG_LINEAR)? D3DFOG_LINEAR : (fogType==EFT_FOG_EXP)?D3DFOG_EXP:D3DFOG_EXP2);

	if (fogType==EFT_FOG_LINEAR)
	{
		pID3DDevice->SetRenderState(D3DRS_FOGSTART, F2DW(start));
		pID3DDevice->SetRenderState(D3DRS_FOGEND, F2DW(end));
	}
	else
		pID3DDevice->SetRenderState(D3DRS_FOGDENSITY, F2DW(density));

	if(!pixelFog)
		pID3DDevice->SetRenderState(D3DRS_RANGEFOGENABLE, rangeFog);
}


//! Draws a 3d line.
void CD3D9Driver::draw3DLine(const core::vector3df& start,
	const core::vector3df& end, SColor color)
{
	setVertexShader(EVT_STANDARD);
	setRenderStates3DMode();
	video::S3DVertex v[2];
	v[0].Color = color;
	v[1].Color = color;
	v[0].Pos = start;
	v[1].Pos = end;

	pID3DDevice->DrawPrimitiveUP(D3DPT_LINELIST, 1, v, sizeof(S3DVertex));
}


//! resets the device
bool CD3D9Driver::reset()
{
	u32 i;
	os::Printer::log("Resetting D3D9 device.", ELL_INFORMATION);

	for (i=0; i<Textures.size(); ++i)
	{
		if (Textures[i].Surface->isRenderTarget())
		{
			IDirect3DBaseTexture9* tex = ((CD3D9Texture*)(Textures[i].Surface))->getDX9Texture();
			if (tex)
				tex->Release();
		}
	}
	for (i=0; i<DepthBuffers.size(); ++i)
	{
		if (DepthBuffers[i]->Surface)
			DepthBuffers[i]->Surface->Release();
	}
	for (i=0; i<OcclusionQueries.size(); ++i)
	{
		if (OcclusionQueries[i].PID)
		{
			reinterpret_cast<IDirect3DQuery9*>(OcclusionQueries[i].PID)->Release();
			OcclusionQueries[i].PID=0;
		}
	}
	// this does not require a restore in the reset method, it's updated
	// automatically in the next render cycle.
	removeAllHardwareBuffers();

	DriverWasReset=true;

	HRESULT hr = pID3DDevice->Reset(&present);

	// restore RTTs
	for (i=0; i<Textures.size(); ++i)
	{
		if (Textures[i].Surface->isRenderTarget())
			((CD3D9Texture*)(Textures[i].Surface))->createRenderTarget();
	}

	// restore screen depthbuffer
	pID3DDevice->GetDepthStencilSurface(&(DepthBuffers[0]->Surface));
	D3DSURFACE_DESC desc;
	// restore other depth buffers
	// depth format is taken from main depth buffer
	DepthBuffers[0]->Surface->GetDesc(&desc);
	// multisampling is taken from rendertarget
	D3DSURFACE_DESC desc2;
	for (i=1; i<DepthBuffers.size(); ++i)
	{
		for (u32 j=0; j<Textures.size(); ++j)
		{
			// all textures sharing this depth buffer must have the same setting
			// so take first one
			if (((CD3D9Texture*)(Textures[j].Surface))->DepthSurface==DepthBuffers[i])
			{
				((CD3D9Texture*)(Textures[j].Surface))->Texture->GetLevelDesc(0,&desc2);
				break;
			}
		}

		pID3DDevice->CreateDepthStencilSurface(DepthBuffers[i]->Size.Width,
				DepthBuffers[i]->Size.Height,
				desc.Format,
				desc2.MultiSampleType,
				desc2.MultiSampleQuality,
				TRUE,
				&(DepthBuffers[i]->Surface),
				NULL);
	}
	for (i=0; i<OcclusionQueries.size(); ++i)
	{
		pID3DDevice->CreateQuery(D3DQUERYTYPE_OCCLUSION, reinterpret_cast<IDirect3DQuery9**>(&OcclusionQueries[i].PID));
	}

	if (FAILED(hr))
	{
		if (hr == D3DERR_DEVICELOST)
		{
			DeviceLost = true;
			os::Printer::log("Resetting failed due to device lost.", ELL_WARNING);
		}
#ifdef D3DERR_DEVICEREMOVED
		else if (hr == D3DERR_DEVICEREMOVED)
		{
			os::Printer::log("Resetting failed due to device removed.", ELL_WARNING);
		}
#endif
		else if (hr == D3DERR_DRIVERINTERNALERROR)
		{
			os::Printer::log("Resetting failed due to internal error.", ELL_WARNING);
		}
		else if (hr == D3DERR_OUTOFVIDEOMEMORY)
		{
			os::Printer::log("Resetting failed due to out of memory.", ELL_WARNING);
		}
		else if (hr == D3DERR_DEVICENOTRESET)
		{
			os::Printer::log("Resetting failed due to not reset.", ELL_WARNING);
		}
		else if (hr == D3DERR_INVALIDCALL)
		{
			os::Printer::log("Resetting failed due to invalid call", "You need to release some more surfaces.", ELL_WARNING);
		}
		else
		{
			os::Printer::log("Resetting failed due to unknown reason.", core::stringc((int)hr).c_str(), ELL_WARNING);
		}
		return false;
	}

	DeviceLost = false;
	ResetRenderStates = true;
	LastVertexType = (E_VERTEX_TYPE)-1;

	for (u32 i=0; i<MATERIAL_MAX_TEXTURES; ++i)
		CurrentTexture[i] = 0;

	setVertexShader(EVT_STANDARD);
	setRenderStates3DMode();
	setFog(FogColor, FogType, FogStart, FogEnd, FogDensity, PixelFog, RangeFog);
	setAmbientLight(AmbientLight);

	return true;
}


void CD3D9Driver::OnResize(const core::dimension2d<u32>& size)
{
	if (!pID3DDevice)
		return;

	CNullDriver::OnResize(size);
	present.BackBufferWidth = size.Width;
	present.BackBufferHeight = size.Height;

	reset();
}


//! Returns type of video driver
E_DRIVER_TYPE CD3D9Driver::getDriverType() const
{
	return EDT_DIRECT3D9;
}


//! Returns the transformation set by setTransform
const core::matrix4& CD3D9Driver::getTransform(E_TRANSFORMATION_STATE state) const
{
	return Matrices[state];
}


//! Sets a vertex shader constant.
void CD3D9Driver::setVertexShaderConstant(const f32* data, s32 startRegister, s32 constantAmount)
{
	if (data)
		pID3DDevice->SetVertexShaderConstantF(startRegister, data, constantAmount);
}


//! Sets a pixel shader constant.
void CD3D9Driver::setPixelShaderConstant(const f32* data, s32 startRegister, s32 constantAmount)
{
	if (data)
		pID3DDevice->SetPixelShaderConstantF(startRegister, data, constantAmount);
}


//! Sets a constant for the vertex shader based on a name.
bool CD3D9Driver::setVertexShaderConstant(const c8* name, const f32* floats, int count)
{
	if (Material.MaterialType >= 0 && Material.MaterialType < (s32)MaterialRenderers.size())
	{
		CD3D9MaterialRenderer* r = (CD3D9MaterialRenderer*)MaterialRenderers[Material.MaterialType].Renderer;
		return r->setVariable(true, name, floats, count);
	}

	return false;
}


//! Bool interface for the above.
bool CD3D9Driver::setVertexShaderConstant(const c8* name, const bool* bools, int count)
{
	if (Material.MaterialType >= 0 && Material.MaterialType < (s32)MaterialRenderers.size())
	{
		CD3D9MaterialRenderer* r = (CD3D9MaterialRenderer*)MaterialRenderers[Material.MaterialType].Renderer;
		return r->setVariable(true, name, bools, count);
	}

	return false;
}


//! Int interface for the above.
bool CD3D9Driver::setVertexShaderConstant(const c8* name, const s32* ints, int count)
{
	if (Material.MaterialType >= 0 && Material.MaterialType < (s32)MaterialRenderers.size())
	{
		CD3D9MaterialRenderer* r = (CD3D9MaterialRenderer*)MaterialRenderers[Material.MaterialType].Renderer;
		return r->setVariable(true, name, ints, count);
	}

	return false;
}


//! Sets a constant for the pixel shader based on a name.
bool CD3D9Driver::setPixelShaderConstant(const c8* name, const f32* floats, int count)
{
	if (Material.MaterialType >= 0 && Material.MaterialType < (s32)MaterialRenderers.size())
	{
		CD3D9MaterialRenderer* r = (CD3D9MaterialRenderer*)MaterialRenderers[Material.MaterialType].Renderer;
		return r->setVariable(false, name, floats, count);
	}

	return false;
}


//! Bool interface for the above.
bool CD3D9Driver::setPixelShaderConstant(const c8* name, const bool* bools, int count)
{
	if (Material.MaterialType >= 0 && Material.MaterialType < (s32)MaterialRenderers.size())
	{
		CD3D9MaterialRenderer* r = (CD3D9MaterialRenderer*)MaterialRenderers[Material.MaterialType].Renderer;
		return r->setVariable(false, name, bools, count);
	}

	return false;
}


//! Int interface for the above.
bool CD3D9Driver::setPixelShaderConstant(const c8* name, const s32* ints, int count)
{
	if (Material.MaterialType >= 0 && Material.MaterialType < (s32)MaterialRenderers.size())
	{
		CD3D9MaterialRenderer* r = (CD3D9MaterialRenderer*)MaterialRenderers[Material.MaterialType].Renderer;
		return r->setVariable(false, name, ints, count);
	}

	return false;
}


//! Adds a new material renderer to the VideoDriver, using pixel and/or
//! vertex shaders to render geometry.
s32 CD3D9Driver::addShaderMaterial(const c8* vertexShaderProgram,
	const c8* pixelShaderProgram,
	IShaderConstantSetCallBack* callback,
	E_MATERIAL_TYPE baseMaterial, s32 userData)
{
	s32 nr = -1;
	CD3D9ShaderMaterialRenderer* r = new CD3D9ShaderMaterialRenderer(
		pID3DDevice, this, nr, vertexShaderProgram, pixelShaderProgram,
		callback, getMaterialRenderer(baseMaterial), userData);

	r->drop();
	return nr;
}


//! Adds a new material renderer to the VideoDriver, based on a high level shading
//! language.
s32 CD3D9Driver::addHighLevelShaderMaterial(
		const c8* vertexShaderProgram,
		const c8* vertexShaderEntryPointName,
		E_VERTEX_SHADER_TYPE vsCompileTarget,
		const c8* pixelShaderProgram,
		const c8* pixelShaderEntryPointName,
		E_PIXEL_SHADER_TYPE psCompileTarget,
		const c8* geometryShaderProgram,
		const c8* geometryShaderEntryPointName,
		E_GEOMETRY_SHADER_TYPE gsCompileTarget,
		scene::E_PRIMITIVE_TYPE inType, scene::E_PRIMITIVE_TYPE outType,
		u32 verticesOut,
		IShaderConstantSetCallBack* callback,
		E_MATERIAL_TYPE baseMaterial, s32 userData, E_GPU_SHADING_LANGUAGE shadingLang)
{
	s32 nr = -1;

	#ifdef _IRR_COMPILE_WITH_CG_
	if(shadingLang == EGSL_CG)
	{
		CD3D9CgMaterialRenderer* r = new CD3D9CgMaterialRenderer(
			this, nr,
			vertexShaderProgram, vertexShaderEntryPointName, vsCompileTarget,
			pixelShaderProgram, pixelShaderEntryPointName, psCompileTarget,
			geometryShaderProgram, geometryShaderEntryPointName, gsCompileTarget,
			inType, outType, verticesOut,
			callback,getMaterialRenderer(baseMaterial), userData);

		r->drop();
	}
	else
	#endif
	{
		CD3D9HLSLMaterialRenderer* r = new CD3D9HLSLMaterialRenderer(
			pID3DDevice, this, nr,
			vertexShaderProgram,
			vertexShaderEntryPointName,
			vsCompileTarget,
			pixelShaderProgram,
			pixelShaderEntryPointName,
			psCompileTarget,
			callback,
			getMaterialRenderer(baseMaterial),
			userData);

		r->drop();
	}

	return nr;
}


//! Returns a pointer to the IVideoDriver interface. (Implementation for
//! IMaterialRendererServices)
IVideoDriver* CD3D9Driver::getVideoDriver()
{
	return this;
}


//! Creates a render target texture.
ITexture* CD3D9Driver::addRenderTargetTexture(const core::dimension2d<u32>& size,
											  const io::path& name,
											  const ECOLOR_FORMAT format)
{
	CD3D9Texture* tex = new CD3D9Texture(this, size, name, format);
	if (tex)
	{
		if (!tex->Texture)
		{
			tex->drop();
			return 0;
		}
		checkDepthBuffer(tex);
		addTexture(tex);
		tex->drop();
	}
	return tex;
}


//! Clears the ZBuffer.
void CD3D9Driver::clearZBuffer()
{
	HRESULT hr = pID3DDevice->Clear( 0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0, 0);

	if (FAILED(hr))
		os::Printer::log("CD3D9Driver clearZBuffer() failed.", ELL_WARNING);
}


//! Returns an image created from the last rendered frame.
IImage* CD3D9Driver::createScreenShot(video::ECOLOR_FORMAT format, video::E_RENDER_TARGET target)
{
	if (target != video::ERT_FRAME_BUFFER)
		return 0;

	// query the screen dimensions of the current adapter
	D3DDISPLAYMODE displayMode;
	pID3DDevice->GetDisplayMode(0, &displayMode);

	if (format==video::ECF_UNKNOWN)
		format=video::ECF_A8R8G8B8;

	// create the image surface to store the front buffer image [always A8R8G8B8]
	HRESULT hr;
	LPDIRECT3DSURFACE9 lpSurface;
	if (FAILED(hr = pID3DDevice->CreateOffscreenPlainSurface(displayMode.Width, displayMode.Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &lpSurface, 0)))
		return 0;

	// read the front buffer into the image surface
	if (FAILED(hr = pID3DDevice->GetFrontBufferData(0, lpSurface)))
	{
		lpSurface->Release();
		return 0;
	}

	RECT clientRect;
	{
		POINT clientPoint;
		clientPoint.x = 0;
		clientPoint.y = 0;

		ClientToScreen((HWND)getExposedVideoData().D3D9.HWnd, &clientPoint);

		clientRect.left   = clientPoint.x;
		clientRect.top	= clientPoint.y;
		clientRect.right  = clientRect.left + ScreenSize.Width;
		clientRect.bottom = clientRect.top  + ScreenSize.Height;

		// window can be off-screen partly, we can't take screenshots from that
		clientRect.left = core::max_(clientRect.left, 0l);
		clientRect.top = core::max_(clientRect.top, 0l);
		clientRect.right = core::min_(clientRect.right, (long)displayMode.Width);
		clientRect.bottom = core::min_(clientRect.bottom, (long)displayMode.Height );
	}

	// lock our area of the surface
	D3DLOCKED_RECT lockedRect;
	if (FAILED(lpSurface->LockRect(&lockedRect, &clientRect, D3DLOCK_READONLY)))
	{
		lpSurface->Release();
		return 0;
	}

	irr::core::dimension2d<u32> shotSize;
	shotSize.Width = core::min_( ScreenSize.Width, (u32)(clientRect.right-clientRect.left) );
	shotSize.Height = core::min_( ScreenSize.Height, (u32)(clientRect.bottom-clientRect.top) );

	// this could throw, but we aren't going to worry about that case very much
	IImage* newImage = createImage(format, shotSize);

	if (newImage)
	{
		// d3d pads the image, so we need to copy the correct number of bytes
		u32* dP = (u32*)newImage->lock();
		u8 * sP = (u8 *)lockedRect.pBits;

		// If the display mode format doesn't promise anything about the Alpha value
		// and it appears that it's not presenting 255, then we should manually
		// set each pixel alpha value to 255.
		if (D3DFMT_X8R8G8B8 == displayMode.Format && (0xFF000000 != (*dP & 0xFF000000)))
		{
			for (u32 y = 0; y < shotSize.Height; ++y)
			{
				for (u32 x = 0; x < shotSize.Width; ++x)
				{
					newImage->setPixel(x,y,*((u32*)sP) | 0xFF000000);
					sP += 4;
				}

				sP += lockedRect.Pitch - (4 * shotSize.Width);
			}
		}
		else
		{
			for (u32 y = 0; y < shotSize.Height; ++y)
			{
				convertColor(sP, video::ECF_A8R8G8B8, shotSize.Width, dP, format);
				sP += lockedRect.Pitch;
				dP += shotSize.Width;
			}
		}

		newImage->unlock();
	}

	// we can unlock and release the surface
	lpSurface->UnlockRect();

	// release the image surface
	lpSurface->Release();

	// return status of save operation to caller
	return newImage;
}


//! returns color format
ECOLOR_FORMAT CD3D9Driver::getColorFormat() const
{
	return ColorFormat;
}


//! returns color format
D3DFORMAT CD3D9Driver::getD3DColorFormat() const
{
	return D3DColorFormat;
}


// returns the current size of the screen or rendertarget
const core::dimension2d<u32>& CD3D9Driver::getCurrentRenderTargetSize() const
{
	if ( CurrentRendertargetSize.Width == 0 )
		return ScreenSize;
	else
		return CurrentRendertargetSize;
}


// Set/unset a clipping plane.
bool CD3D9Driver::setClipPlane(u32 index, const core::plane3df& plane, bool enable)
{
	if (index >= MaxUserClipPlanes)
		return false;

	HRESULT ok = pID3DDevice->SetClipPlane(index, (const float*)&(plane.Normal.X));
	if (D3D_OK == ok)
		enableClipPlane(index, enable);
	return true;
}


// Enable/disable a clipping plane.
void CD3D9Driver::enableClipPlane(u32 index, bool enable)
{
	if (index >= MaxUserClipPlanes)
		return;
	DWORD renderstate;
	HRESULT ok = pID3DDevice->GetRenderState(D3DRS_CLIPPLANEENABLE, &renderstate);
	if (S_OK == ok)
	{
		if (enable)
			renderstate |= (1 << index);
		else
			renderstate &= ~(1 << index);
		ok = pID3DDevice->SetRenderState(D3DRS_CLIPPLANEENABLE, renderstate);
	}
}


D3DFORMAT CD3D9Driver::getD3DFormatFromColorFormat(ECOLOR_FORMAT format) const
{
	switch(format)
	{
		case ECF_A1R5G5B5:
			return D3DFMT_A1R5G5B5;
		case ECF_R5G6B5:
			return D3DFMT_R5G6B5;
		case ECF_R8G8B8:
			return D3DFMT_R8G8B8;
		case ECF_A8R8G8B8:
			return D3DFMT_A8R8G8B8;

		// Floating Point formats. Thanks to Patryk "Nadro" Nadrowski.
		case ECF_R16F:
			return D3DFMT_R16F;
		case ECF_G16R16F:
			return D3DFMT_G16R16F;
		case ECF_A16B16G16R16F:
			return D3DFMT_A16B16G16R16F;
		case ECF_R32F:
			return D3DFMT_R32F;
		case ECF_G32R32F:
			return D3DFMT_G32R32F;
		case ECF_A32B32G32R32F:
			return D3DFMT_A32B32G32R32F;
	}
	return D3DFMT_UNKNOWN;
}


ECOLOR_FORMAT CD3D9Driver::getColorFormatFromD3DFormat(D3DFORMAT format) const
{
	switch(format)
	{
		case D3DFMT_X1R5G5B5:
		case D3DFMT_A1R5G5B5:
			return ECF_A1R5G5B5;
		case D3DFMT_A8B8G8R8:
		case D3DFMT_A8R8G8B8:
		case D3DFMT_X8R8G8B8:
			return ECF_A8R8G8B8;
		case D3DFMT_R5G6B5:
			return ECF_R5G6B5;
		case D3DFMT_R8G8B8:
			return ECF_R8G8B8;

		// Floating Point formats. Thanks to Patryk "Nadro" Nadrowski.
		case D3DFMT_R16F:
			return ECF_R16F;
		case D3DFMT_G16R16F:
			return ECF_G16R16F;
		case D3DFMT_A16B16G16R16F:
			return ECF_A16B16G16R16F;
		case D3DFMT_R32F:
			return ECF_R32F;
		case D3DFMT_G32R32F:
			return ECF_G32R32F;
		case D3DFMT_A32B32G32R32F:
			return ECF_A32B32G32R32F;
		default:
			return (ECOLOR_FORMAT)0;
	};
}


void CD3D9Driver::checkDepthBuffer(ITexture* tex)
{
	if (!tex)
		return;
	const core::dimension2du optSize = tex->getSize().getOptimalSize(
			!queryFeature(EVDF_TEXTURE_NPOT),
			!queryFeature(EVDF_TEXTURE_NSQUARE), true);
	SDepthSurface* depth=0;
	core::dimension2du destSize(0x7fffffff, 0x7fffffff);
	for (u32 i=0; i<DepthBuffers.size(); ++i)
	{
		if ((DepthBuffers[i]->Size.Width>=optSize.Width) &&
			(DepthBuffers[i]->Size.Height>=optSize.Height))
		{
			if ((DepthBuffers[i]->Size.Width<destSize.Width) &&
				(DepthBuffers[i]->Size.Height<destSize.Height))
			{
				depth = DepthBuffers[i];
				destSize=DepthBuffers[i]->Size;
			}
		}
	}
	if (!depth)
	{
		D3DSURFACE_DESC desc;
		DepthBuffers[0]->Surface->GetDesc(&desc);
		// the multisampling needs to match the RTT
		D3DSURFACE_DESC desc2;
		((CD3D9Texture*)tex)->Texture->GetLevelDesc(0,&desc2);
		DepthBuffers.push_back(new SDepthSurface());
		HRESULT hr=pID3DDevice->CreateDepthStencilSurface(optSize.Width,
				optSize.Height,
				desc.Format,
				desc2.MultiSampleType,
				desc2.MultiSampleQuality,
				TRUE,
				&(DepthBuffers.getLast()->Surface),
				NULL);
		if (SUCCEEDED(hr))
		{
			depth=DepthBuffers.getLast();
			depth->Surface->GetDesc(&desc);
			depth->Size.set(desc.Width, desc.Height);
		}
		else
		{
			if (hr == D3DERR_OUTOFVIDEOMEMORY)
				os::Printer::log("Could not create DepthBuffer","out of video memory",ELL_ERROR);
			else if( hr == E_OUTOFMEMORY )
				os::Printer::log("Could not create DepthBuffer","out of memory",ELL_ERROR);
			else
			{
				char buffer[128];
				sprintf(buffer,"Could not create DepthBuffer of %ix%i",optSize.Width,optSize.Height);
				os::Printer::log(buffer,ELL_ERROR);
			}
			DepthBuffers.erase(DepthBuffers.size()-1);
		}
	}
	else
		depth->grab();

	static_cast<CD3D9Texture*>(tex)->DepthSurface=depth;
}


void CD3D9Driver::removeDepthSurface(SDepthSurface* depth)
{
	for (u32 i=0; i<DepthBuffers.size(); ++i)
	{
		if (DepthBuffers[i]==depth)
		{
			DepthBuffers.erase(i);
			return;
		}
	}
}


core::dimension2du CD3D9Driver::getMaxTextureSize() const
{
	return core::dimension2du(Caps.MaxTextureWidth, Caps.MaxTextureHeight);
}

#ifdef _IRR_COMPILE_WITH_CG_
const CGcontext& CD3D9Driver::getCgContext()
{
	return CgContext;
}
#endif


} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_DIRECT3D_9_



namespace irr
{
namespace video
{

#ifdef _IRR_COMPILE_WITH_DIRECT3D_9_
//! creates a video driver
IVideoDriver* createDirectX9Driver(const SIrrlichtCreationParameters& params,
			io::IFileSystem* io, HWND window)
{
	const bool pureSoftware = false;
	CD3D9Driver* dx9 = new CD3D9Driver(params, io);
	if (!dx9->initDriver(window, pureSoftware))
	{
		dx9->drop();
		dx9 = 0;
	}

	return dx9;
}
#endif // _IRR_COMPILE_WITH_DIRECT3D_9_

} // end namespace video
} // end namespace irr

