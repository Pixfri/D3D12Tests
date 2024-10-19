// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of D3D12 Tests.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#ifndef D3D12TESTS_HELLOWINDOW_HELLOWINDOW_HPP
#define D3D12TESTS_HELLOWINDOW_HELLOWINDOW_HPP

#include "Framework/Application.hpp"

#include "Framework/pch.hpp"

namespace HelloWindow {
    // Note that while ComPtr is used to manage the lifetime of resources on the CPU,
    // it has no understanding of the lifetime of resources on the GPU. Apps must account
    // for the GPU lifetime of resources to avoid destroying objects that may still be
    // referenced by the GPU.
    // An example of this can be found in the class method: OnDestroy().
    using Microsoft::WRL::ComPtr;

    class HelloWindow : public D3D12Tests::Application {
    public:
        HelloWindow(UINT width, UINT height, const std::wstring& name);
        ~HelloWindow() override = default;

        HelloWindow(const HelloWindow&) = delete;
        HelloWindow(HelloWindow&&) = delete;

        HelloWindow& operator=(const HelloWindow&) = delete;
        HelloWindow& operator=(HelloWindow&&) = delete;

        void OnInit() override;
        void OnUpdate() override;
        void OnRender() override;
        void OnDestroy() override;

    private:
        static const UINT FrameCount = 2;

        // Pipeline objects.
        ComPtr<IDXGISwapChain3> m_SwapChain;
        ComPtr<ID3D12Device> m_Device;
        ComPtr<ID3D12Resource> m_RenderTargets[FrameCount];
        ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
        ComPtr<ID3D12CommandQueue> m_CommandQueue;
        ComPtr<ID3D12DescriptorHeap> m_RtvHeap;
        ComPtr<ID3D12PipelineState> m_PipelineState;
        ComPtr<ID3D12GraphicsCommandList> m_CommandList;
        UINT m_RtvDescriptorSize;

        // Synchronization objects.
        UINT m_FrameIndex;
        HANDLE m_FenceEvent;
        ComPtr<ID3D12Fence> m_Fence;
        UINT64 m_FenceValue;

        void LoadPipeline();
        void LoadAssets();
        void PopulateCommandList();
        void WaitForPreviousFrame();
    };
}

#include "HelloWindow/HelloWindow.inl"

#endif // D3D12TESTS_HELLOWINDOW_HELLOWINDOW_HPP
