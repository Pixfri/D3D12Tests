// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of D3D12 Tests.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#ifndef D3D12TESTS_WIN32APPLICATIONBASE_HPP
#define D3D12TESTS_WIN32APPLICATIONBASE_HPP

#include "Framework/pch.hpp"

#include "Framework/Application.hpp"

namespace D3D12Tests {
    class Application;

    class Win32ApplicationBase {
    public:
        static Int32 Run(Application* pApplication, HINSTANCE hInstance, Int32 nCmdShow);
        inline static HWND GetHwnd();
    
    protected:
        static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    private:
        static HWND m_HWnd;
    };
}

#include "Framework/Win32ApplicationBase.inl"

#endif // D3D12TESTS_WIN32APPLICATIONBASE_HPP
