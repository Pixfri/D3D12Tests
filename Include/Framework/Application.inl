#include "Application.hpp"
// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of D3D12 Tests.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

namespace D3D12Tests {
    inline void Application::OnKeyDown(UINT /*key*/) {
    }
    
    inline void Application::OnKeyUp(UINT /*key*/) {
    }

    // Accessors
    inline UINT Application::GetWidth() const {
        return m_Width;
    }
    
    inline UINT Application::GetHeight() const {
        return m_Height;
    }
    
    inline const WCHAR* Application::GetTitle() const {
        return m_Title.c_str();
    }

    inline std::wstring Application::GetAssetFullPath(LPCWSTR assetName) const {
        return m_AssetsPath + assetName;
    }
}
