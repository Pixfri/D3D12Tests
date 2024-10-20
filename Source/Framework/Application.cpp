// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of D3D12 Tests.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "Framework/Application.hpp"

namespace D3D12Tests {
    using namespace Microsoft::WRL;

    Application::Application(UINT width, UINT height, std::wstring name) :
        m_Width(width),
        m_Height(height),
        m_Title(name),
        m_UseWarpDevice(false) {
        WCHAR assetsPath[512];
        GetAssetsPath(assetsPath, _countof(assetsPath));
        m_AssetsPath = assetsPath;

        m_AspectRatio = static_cast<Float32>(width) / static_cast<Float32>(height);
    }

    // Helper function for parsing any supplied command line args.
    _Use_decl_annotations_
    void Application::ParseCommandLineArgs(WCHAR* argv[], Int32 argc) {
        for (Int32 i = 1; i < argc; ++i) {
            if (_wcsnicmp(argv[i], L"-warp", wcslen(argv[i])) == 0 ||
                _wcsnicmp(argv[i], L"/warp", wcslen(argv[i])) == 0) {
                m_UseWarpDevice = true;
                m_Title = m_Title + L" (WARP)";
            }
        }
    }
    
    // Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
    // If no such adapter can be found, *ppAdapter will be set to nullptr.
    _Use_decl_annotations_
    void Application::GetHardwareAdapter(
        IDXGIFactory1* pFactory,
        IDXGIAdapter1** ppAdapter,
        bool requestHighPerformanceAdapter
    ) {
        *ppAdapter = nullptr;

        ComPtr<IDXGIAdapter1> adapter;

        ComPtr<IDXGIFactory6> factory6;
        if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6)))) {
            for (
                UINT adapterIndex = 0;
                SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                    adapterIndex,
                    requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
                    IID_PPV_ARGS(&adapter)));
                ++adapterIndex
            ) {
                DXGI_ADAPTER_DESC1 desc;
                adapter->GetDesc1(&desc);

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                    // Don't selec the Basic Render Device adapter.
                    // If you want a software adapter, pass in '/warp' on the command line.
                    continue;
                }

                // Check to see whether the adapter supports Direct3D 12, but don't create the
                // actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
                    break;
                }
            }
        }

        if (adapter.Get() == nullptr) {
            for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex) {
                DXGI_ADAPTER_DESC1 desc;
                adapter->GetDesc1(&desc);


                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                    // Don't selec the Basic Render Device adapter.
                    // If you want a software adapter, pass in '/warp' on the command line.
                    continue;
                }

                // Check to see whether the adapter supports Direct3D 12, but don't create the
                // actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
                    break;
                }
            }
        }

        *ppAdapter = adapter.Detach();
    }

    void Application::SetCustomWindowText(LPCWSTR text) const {
        std::wstring windowText = m_Title + L": " + text;
        SetWindowText(Win32ApplicationBase::GetHwnd(), windowText.c_str());
    }
}
