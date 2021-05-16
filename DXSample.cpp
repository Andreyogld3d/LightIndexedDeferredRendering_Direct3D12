//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#include "stdafx.h"
#include "DXSample.h"
#include <comdef.h>
#ifndef NDEBUG
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#include <dxgidebug.h>
#endif

using namespace Microsoft::WRL;

void OutDirect3DError(HRESULT hr)
{
	if (S_OK == hr) {
		return;
	}
	const char* Description = " Description: ";
	_com_error err(hr);
	char description[256];
	char* message = description;
	const wchar_t* desc = err.Description();
	if (desc) {
		while (*message++ = static_cast<char>(*desc++));
		LogMsgErr("Direct3D error %s%s%s\n", description, Description, err.ErrorMessage());
	}
	else {
		LogMsgErr("Direct3D error %s%s\n", Description, err.ErrorMessage());
	}
}

DXSample::DXSample(UINT width, UINT height, std::wstring name) :
    m_width(width),
    m_height(height),
    m_title(name),
    m_useWarpDevice(false)
{
    WCHAR assetsPath[512];
    GetAssetsPath(assetsPath, _countof(assetsPath));
    m_assetsPath = assetsPath;

    m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

DXSample::~DXSample()
{
}

// Helper function for resolving the full path of assets.
std::wstring DXSample::GetAssetFullPath(LPCWSTR assetName)
{
    return m_assetsPath + assetName;
}

void DXSample::InitDebug()
{
	using namespace	Microsoft::WRL;
#ifndef NDEBUG
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	ComPtr<ID3D12DebugDevice> pID3D12DebugDevice;
	HRESULT hr = D3D12GetDebugInterface(IID_PPV_ARGS(&pID3D12DebugDevice));
	if (SUCCEEDED(hr)) {
		hr = pID3D12DebugDevice->SetFeatureMask(D3D12_DEBUG_FEATURE_ALLOW_BEHAVIOR_CHANGING_DEBUG_AIDS | D3D12_DEBUG_FEATURE_CONSERVATIVE_RESOURCE_STATE_TRACKING);
		OutDirect3DError(hr);
	}
	ComPtr<ID3D12DebugDevice1> pID3D12DebugDevice1;
	hr = D3D12GetDebugInterface(IID_PPV_ARGS(&pID3D12DebugDevice1));
	if (SUCCEEDED(hr)) {
		hr = pID3D12DebugDevice1->SetDebugParameter(D3D12_DEBUG_DEVICE_PARAMETER_GPU_BASED_VALIDATION_SETTINGS, nullptr, 0);
		OutDirect3DError(hr);
	}
#ifdef NTDDI_WIN10_RS4
	ComPtr<ID3D12DebugDevice2> pID3D12DebugDevice2;
	hr = D3D12GetDebugInterface(IID_PPV_ARGS(&pID3D12DebugDevice2));
	if (SUCCEEDED(hr)) {
		hr = pID3D12DebugDevice2->SetDebugParameter(D3D12_DEBUG_DEVICE_PARAMETER_GPU_BASED_VALIDATION_SETTINGS, nullptr, 0);
		OutDirect3DError(hr);
	}
#endif
	ComPtr<ID3D12Debug> debugController;
	hr = D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
	OutDirect3DError(hr);
	if (FAILED(hr)) {
		return;
	}
	debugController->EnableDebugLayer();
#ifdef NTDDI_WIN10_19H1
	ComPtr<ID3D12DeviceRemovedExtendedDataSettings> pDredSettings;
	hr = D3D12GetDebugInterface(IID_PPV_ARGS(&pDredSettings));
	OutDirect3DError(hr);
	if (SUCCEEDED(hr)) {
		// Turn on auto-breadcrumbs and page fault reporting.
		pDredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
		pDredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
	}
#endif
	ComPtr<ID3D12Debug1> debug1;
	hr = debugController->QueryInterface(IID_PPV_ARGS(&debug1));
	OutDirect3DError(hr);
	if (FAILED(hr)) {
		return;
	}
	debug1->SetEnableGPUBasedValidation(TRUE);
	debug1->SetEnableSynchronizedCommandQueueValidation(TRUE);

	ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue)))) {
		dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
		dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
		dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, true);

		DXGI_INFO_QUEUE_MESSAGE_ID hide[] = {
			80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
		};
		DXGI_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = countof(hide);
		filter.DenyList.pIDList = hide;
		dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
	}
#ifdef NTDDI_WIN10_RS5
	ComPtr<ID3D12Debug2> debug2;
	hr = debug1->QueryInterface(IID_PPV_ARGS(&debug2));
	OutDirect3DError(hr);
	if (FAILED(hr)) {
		return;
	}
	debug2->SetGPUBasedValidationFlags(D3D12_GPU_BASED_VALIDATION_FLAGS_DISABLE_STATE_TRACKING);
	ComPtr<ID3D12Debug3> debug3;
	hr = debug2->QueryInterface(IID_PPV_ARGS(&debug3));
	OutDirect3DError(hr);
	if (FAILED(hr)) {
		return;
	}
	debug3->EnableDebugLayer();
	debug3->SetEnableGPUBasedValidation(TRUE);
	debug3->SetGPUBasedValidationFlags(D3D12_GPU_BASED_VALIDATION_FLAGS_DISABLE_STATE_TRACKING);
	debug3->SetEnableSynchronizedCommandQueueValidation(TRUE);
#endif
#endif
}

// Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be found, *ppAdapter will be set to nullptr.
_Use_decl_annotations_
void DXSample::GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
{
    ComPtr<IDXGIAdapter1> adapter;
    *ppAdapter = nullptr;

    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter.
            // If you want a software adapter, pass in "/warp" on the command line.
            continue;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create the
        // actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
        {
            break;
        }
    }

    *ppAdapter = adapter.Detach();
}

// Helper function for setting the window's title text.
void DXSample::SetCustomWindowText(LPCWSTR text)
{
    std::wstring windowText = m_title + L": " + text;
    SetWindowText(Win32Application::GetHwnd(), windowText.c_str());
}

// Helper function for parsing any supplied command line args.
_Use_decl_annotations_
void DXSample::ParseCommandLineArgs(WCHAR* argv[], int argc)
{
    for (int i = 1; i < argc; ++i)
    {
        if (_wcsnicmp(argv[i], L"-warp", wcslen(argv[i])) == 0 || 
            _wcsnicmp(argv[i], L"/warp", wcslen(argv[i])) == 0)
        {
            m_useWarpDevice = true;
            m_title = m_title + L" (WARP)";
        }
    }
}
