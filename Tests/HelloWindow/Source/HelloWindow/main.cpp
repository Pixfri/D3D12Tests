// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of D3D12 Tests.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "HelloWindow/HelloWindow.hpp"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    HelloWindow::HelloWindow test(1280, 720, L"D3D12 Hello Window");
    return D3D12Tests::Win32ApplicationBase::Run(&test, hInstance, nCmdShow);
}