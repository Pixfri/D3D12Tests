// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of D3D12 Tests.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#ifndef D3D12TESTS_HELLOTEXTURE_HELLOTEXTURE_HPP
#define D3D12TESTS_HELLOTEXTURE_HELLOTEXTURE_HPP

#include "Framework/Application.hpp"

#include "Framework/pch.hpp"

namespace HelloTexture {
    // Note that while ComPtr is used to manage the lifetime of resources on the CPU,
    // it has no understanding of the lifetime of resources on the GPU. Apps must account
    // for the GPU lifetime of resources to avoid destroying objects that may still be
    // referenced by the GPU.
    // An example of this can be found in the class method: OnDestroy().
    using Microsoft::WRL::ComPtr;

    class HelloTexture : public D3D12Tests::Application {
    public:
        HelloTexture(UINT width, UINT height, const std::wstring& name);
        ~HelloTexture() override = default;

        HelloTexture(const HelloTexture&) = delete;
        HelloTexture(HelloTexture&&) = delete;

        HelloTexture& operator=(const HelloTexture&) = delete;
        HelloTexture& operator=(HelloTexture&&) = delete;

        void OnInit() override;
        void OnUpdate() override;
        void OnRender() override;
        void OnDestroy() override;

    private:
        static constexpr UINT FrameCount = 2;
        static constexpr UINT TextureWidth = 256;
        static constexpr UINT TextureHeight = 256;
        static constexpr UINT TexturePixelSize = 4; // The number of bytes used to represent a pixel in the texture.

        struct Vertex {
            DirectX::XMFLOAT3 Position;
            DirectX::XMFLOAT2 Uv;
        };

        // Pipeline objects.
        CD3DX12_VIEWPORT m_Viewport;
        CD3DX12_RECT m_ScissorRect;
        ComPtr<IDXGISwapChain3> m_SwapChain;
        ComPtr<ID3D12Device> m_Device;
        ComPtr<ID3D12Resource> m_RenderTargets[FrameCount];
        ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
        ComPtr<ID3D12CommandQueue> m_CommandQueue;
        ComPtr<ID3D12RootSignature> m_RootSignature;
        ComPtr<ID3D12DescriptorHeap> m_RtvHeap;
        ComPtr<ID3D12DescriptorHeap> m_SrvHeap;
        ComPtr<ID3D12PipelineState> m_PipelineState;
        ComPtr<ID3D12GraphicsCommandList> m_CommandList;
        UINT m_RtvDescriptorSize;

        // App resources
        ComPtr<ID3D12Resource> m_VertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
        ComPtr<ID3D12Resource> m_Texture;

        // Synchronization objects.
        UINT m_FrameIndex;
        HANDLE m_FenceEvent;
        ComPtr<ID3D12Fence> m_Fence;
        UINT64 m_FenceValue;

        void LoadPipeline();
        void LoadAssets();
        static std::vector<UINT8> GenerateTextureData();
        void PopulateCommandList();
        void WaitForPreviousFrame();
    };
}

#include "HelloTexture/HelloTexture.inl"

#endif // D3D12TESTS_HELLOTEXTURE_HELLOTEXTURE_HPP
