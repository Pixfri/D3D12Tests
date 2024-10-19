// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of D3D12 Tests.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "HelloWindow/HelloWindow.hpp"

namespace HelloWindow {
    HelloWindow::HelloWindow(const UINT width, const UINT height, const std::wstring& name) :
        D3D12Tests::Application(width, height, name),
        m_RtvDescriptorSize(0),
        m_FrameIndex(0) {
    }

    void HelloWindow::OnInit() {
        LoadPipeline();
        LoadAssets();
    }

    void HelloWindow::OnUpdate() {
    }

    void HelloWindow::OnRender() {
        // Record all the commands we need to render the scene into the command list.
        PopulateCommandList();

        // Execute the command list.
        ID3D12CommandList* ppCommandLists[] = {m_CommandList.Get()};
        m_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // Present the frame.
        D3D12Tests::ThrowIfFailed(m_SwapChain->Present(1, 0));

        WaitForPreviousFrame();
    }

    void HelloWindow::OnDestroy() {
        WaitForPreviousFrame();

        CloseHandle(m_FenceEvent);
    }

    void HelloWindow::LoadPipeline() {
        UINT dxgiFactoryFlags = 0;

#ifdef D3D12TESTS_DEBUG
        // Enable the debug layer (requires the Graphics Tools "optional feature")
        // NOTE: Enabling the debug layer after device creation will invalidate the active device.
        {
            ComPtr<ID3D12Debug> debugController;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
                debugController->EnableDebugLayer();

                // Enable additional debug layers.
                dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
            }
        }
#endif

        ComPtr<IDXGIFactory4> factory;
        D3D12Tests::ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

        if (m_UseWarpDevice) {
            ComPtr<IDXGIAdapter> warpAdapter;
            D3D12Tests::ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

            D3D12Tests::ThrowIfFailed(D3D12CreateDevice(
                warpAdapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS((&m_Device))
            ));
        } else {
            ComPtr<IDXGIAdapter1> hardwareAdapter;
            GetHardwareAdapter(factory.Get(), &hardwareAdapter);

            D3D12Tests::ThrowIfFailed(D3D12CreateDevice(
                hardwareAdapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(&m_Device)
            ));
        }

        // Describe and create the command queue.
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        D3D12Tests::ThrowIfFailed(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue)));

        // Describe and create the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = FrameCount;
        swapChainDesc.Width = m_Width;
        swapChainDesc.Height = m_Height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;

        ComPtr<IDXGISwapChain1> swapChain;
        D3D12Tests::ThrowIfFailed(factory->CreateSwapChainForHwnd(
            m_CommandQueue.Get(), // Swap chain needs the queue so that it can force a flush on it.
            D3D12Tests::Win32ApplicationBase::GetHwnd(),
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain));

        // This test does not support fullscreen transitions.
        D3D12Tests::ThrowIfFailed(
            factory->MakeWindowAssociation(D3D12Tests::Win32ApplicationBase::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

        D3D12Tests::ThrowIfFailed(swapChain.As(&m_SwapChain));
        m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();

        // Create descriptor heaps
        {
            // Describe and create a render target view (RTV) descriptor heap.
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
            rtvHeapDesc.NumDescriptors = FrameCount;
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            D3D12Tests::ThrowIfFailed(m_Device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RtvHeap)));

            m_RtvDescriptorSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        }

        // Create frame resources
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RtvHeap->GetCPUDescriptorHandleForHeapStart());

            // Create an RTV for each frame.
            for (UINT n = 0; n < FrameCount; n++) {
                D3D12Tests::ThrowIfFailed(m_SwapChain->GetBuffer(n, IID_PPV_ARGS(&m_RenderTargets[n])));
                m_Device->CreateRenderTargetView(m_RenderTargets[n].Get(), nullptr, rtvHandle);
                rtvHandle.Offset(1, m_RtvDescriptorSize);
            }
        }

        D3D12Tests::ThrowIfFailed(
            m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator)));
    }

    // Load the test's assets.
    void HelloWindow::LoadAssets() {
        // Create the command list
        D3D12Tests::ThrowIfFailed(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                              m_CommandAllocator.Get(), nullptr,
                                                              IID_PPV_ARGS(&m_CommandList)));

        // Command lists are created in the recording state, but there is nothing
        // to record yet. The main loop expects it to be closed, so close it now.
        D3D12Tests::ThrowIfFailed(m_CommandList->Close());

        // Create synchronization objects.
        {
            D3D12Tests::ThrowIfFailed(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
            m_FenceValue = 1;

            // Create an event handler to use for frame synchronization.
            m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (m_FenceEvent == nullptr) {
                D3D12Tests::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
            }
        }
    }

    void HelloWindow::PopulateCommandList() {
        // Command list allocators can only be reset when the associated
        // command lists have finished executing on the GPU; apps should use
        // fences to determine GPU execution progress.
        D3D12Tests::ThrowIfFailed(m_CommandAllocator->Reset());

        // However, when ExecuteCommandList() is called on a particular command
        // list, the command list can then be reset at any time and must be before
        // re-recording.
        D3D12Tests::ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), m_PipelineState.Get()));

        // ReSharper disable CppMsExtAddressOfClassRValue

        // Indicate that the back buffer will be used as a render target.
        auto transition = CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTargets[m_FrameIndex].Get(),
                                                               D3D12_RESOURCE_STATE_PRESENT,
                                                               D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_CommandList->ResourceBarrier(1, &transition);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RtvHeap->GetCPUDescriptorHandleForHeapStart(), m_FrameIndex,
                                                m_RtvDescriptorSize);

        // Record commands
        constexpr D3D12Tests::Float32 clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
        m_CommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

        // Indicate that the back buffer will now be used to present.
        transition = CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTargets[m_FrameIndex].Get(),
                                                          D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                          D3D12_RESOURCE_STATE_PRESENT);
        m_CommandList->ResourceBarrier(1, &transition);

        D3D12Tests::ThrowIfFailed(m_CommandList->Close());
    }

    void HelloWindow::WaitForPreviousFrame() {
        // WAITING FOR THE FRAME TO BE COMPLETE BEFORE CONTINUING IS NOT THE BEST PRACTICE.
        // This is code implemented as such for simplicity. The HelloFrameBuffering
        // test illustrates how to use fences for efficient resource usage and to
        // maximize GPU utilization.

        // Signal and increment the fence value.
        const UINT64 fence = m_FenceValue;
        D3D12Tests::ThrowIfFailed(m_CommandQueue->Signal(m_Fence.Get(), fence));
        m_FenceValue++;

        // Wait until the previous fence is finished.
        if (m_Fence->GetCompletedValue() < fence) {
            D3D12Tests::ThrowIfFailed(m_Fence->SetEventOnCompletion(fence, m_FenceEvent));
            WaitForSingleObject(m_FenceEvent, INFINITE);
        }

        m_FrameIndex = m_SwapChain->GetCurrentBackBufferIndex();
    }
}
