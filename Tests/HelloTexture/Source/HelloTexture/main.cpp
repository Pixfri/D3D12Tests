// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of D3D12 Tests.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "HelloTexture/HelloTexture.hpp"

_Use_decl_annotations_
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    HelloTexture::HelloTexture test(1280, 720, L"D3D12 Hello Texture");
    return D3D12Tests::Win32ApplicationBase::Run(&test, hInstance, nCmdShow);
}