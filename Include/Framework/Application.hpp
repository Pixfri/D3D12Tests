// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of D3D12 Tests.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#ifndef D3D12TESTS_APPLICATION_HPP
#define D3D12TESTS_APPLICATION_HPP

#include "Framework/pch.hpp"

#include "Framework/ApplicationHelper.hpp"
#include "Framework/Win32ApplicationBase.hpp"

namespace D3D12Tests {
    class Application {
    public:
        Application(UINT width, UINT height, std::wstring name);
        virtual ~Application() = default;

        Application(const Application&) = delete;
        Application(Application&&) = delete;

        Application& operator=(const Application&) = delete;
        Application& operator=(Application&&) = delete;

        virtual void OnInit() = 0;
        virtual void OnUpdate() = 0;
        virtual void OnRender() = 0;
        virtual void OnDestroy() = 0;

        inline virtual void OnKeyDown(UINT key);
        inline virtual void OnKeyUp(UINT key);

        // Accessors
        inline UINT GetWidth() const;
        inline UINT GetHeight() const;
        inline const WCHAR* GetTitle() const;

        void ParseCommandLineArgs(_In_reads_(argc) WCHAR* argv[], Int32 argc);

    protected:
        inline std::wstring GetAssetFullPath(LPCWSTR assetName) const;

        void GetHardwareAdapter(
            _In_ IDXGIFactory1* pFactory,
            _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter,
            bool requestHighPerformanceAdapter = false
        );

        void SetCustomWindowText(LPCWSTR text) const;

        // Viewport dimensions.
        UINT m_Width;
        UINT m_Height;
        Float32 m_AspectRatio;

        // Adapter info.
        bool m_UseWarpDevice;
    
    private:
        // Root assets' path.
        std::wstring m_AssetsPath;

        // Window title.
        std::wstring m_Title;
    };
}

#include "Framework/Application.inl"

#endif // D3D12TESTS_APPLICATION_HPP
