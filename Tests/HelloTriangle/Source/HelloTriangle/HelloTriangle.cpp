// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of D3D12 Tests.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "HelloTriangle/HelloTriangle.hpp"

namespace HelloTriangle {
    HelloTriangle::HelloTriangle(const UINT width, const UINT height, const std::wstring& name) :
        D3D12Tests::Application(width, height, name),
        m_Viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
        m_ScissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
        m_RtvDescriptorSize(0),
        m_FrameIndex(0) {
    }

    void HelloTriangle::OnInit() {
        LoadPipeline();
        LoadAssets();
    }

    void HelloTriangle::OnUpdate() {
    }

    void HelloTriangle::OnRender() {
        // Record all the commands we need to render the scene into the command list.
        PopulateCommandList();

        // Execute the command list.
        ID3D12CommandList* ppCommandLists[] = {m_CommandList.Get()};
        m_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // Present the frame.
        D3D12Tests::ThrowIfFailed(m_SwapChain->Present(1, 0));

        WaitForPreviousFrame();
    }

    void HelloTriangle::OnDestroy() {
        WaitForPreviousFrame();

        CloseHandle(m_FenceEvent);
    }

    void HelloTriangle::LoadPipeline() {
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
    void HelloTriangle::LoadAssets() {
        // Create an empty root signature.
        {
            CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
            rootSignatureDesc.Init(0, nullptr, 0, nullptr,
                                   D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

            ComPtr<ID3DBlob> signature;
            ComPtr<ID3DBlob> error;
            D3D12Tests::ThrowIfFailed(
                D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
            D3D12Tests::ThrowIfFailed(m_Device->CreateRootSignature(0, signature->GetBufferPointer(),
                                                                    signature->GetBufferSize(),
                                                                    IID_PPV_ARGS(&m_RootSignature)));
        }

        // Create the pipeline state, which includes compiling and loading shaders.
        {
            ComPtr<ID3DBlob> vertexShader;
            ComPtr<ID3DBlob> pixelShader;

            UINT compilerFlags = 0;
#ifdef D3D12TESTS_DEBUG
            // Enable better shader debugging with the graphics debugging tools.
            compilerFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

            D3D12Tests::ThrowIfFailed(D3DCompileFromFile(
                GetAssetFullPath(L"HelloTriangle/Resources/Shaders/shader.hlsl").c_str(), nullptr, nullptr, "VSMain",
                "vs_5_0", compilerFlags, 0, &vertexShader, nullptr));
            D3D12Tests::ThrowIfFailed(D3DCompileFromFile(
                GetAssetFullPath(L"HelloTriangle/Resources/Shaders/shader.hlsl").c_str(), nullptr, nullptr, "PSMain",
                "ps_5_0", compilerFlags, 0, &pixelShader, nullptr));

            // Define the vertex input layout
            D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
                {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
            };

            // Describe and create the graphics pipeline state object (PSO).
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.InputLayout = {inputElementDescs, _countof(inputElementDescs)};
            psoDesc.pRootSignature = m_RootSignature.Get();
            psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
            psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
            psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            psoDesc.DepthStencilState.DepthEnable = FALSE;
            psoDesc.DepthStencilState.StencilEnable = FALSE;
            psoDesc.SampleMask = UINT_MAX;
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            psoDesc.SampleDesc.Count = 1;
            D3D12Tests::ThrowIfFailed(m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PipelineState)));
        }

        // Create the command list
        D3D12Tests::ThrowIfFailed(m_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                              m_CommandAllocator.Get(), nullptr,
                                                              IID_PPV_ARGS(&m_CommandList)));

        // Command lists are created in the recording state, but there is nothing
        // to record yet. The main loop expects it to be closed, so close it now.
        D3D12Tests::ThrowIfFailed(m_CommandList->Close());

        // Create the vertex buffer.
        {
            // Define the geometry for a triangle.
            Vertex triangleVertices[] = {
                {{0.0f, 0.25f * m_AspectRatio, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
                {{0.25f, -0.25f * m_AspectRatio, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
                {{-0.25f, -0.25f * m_AspectRatio, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            };

            const UINT vertexBufferSize = sizeof(triangleVertices);

            // Note: using upload heaps to transfer static data like vert buffers is not 
            // recommended. Every time the GPU needs it, the upload heap will be marshalled 
            // over. Please read up on Default Heap usage. An upload heap is used here for 
            // code simplicity and because there are very few verts to actually transfer.
            auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
            D3D12Tests::ThrowIfFailed(m_Device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_VertexBuffer)));

            // Copy the triangle data to the vertex buffer.
            UINT8* pVertexDataBegin;
            CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resources on the CPU.
            D3D12Tests::ThrowIfFailed(m_VertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
            memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
            m_VertexBuffer->Unmap(0, nullptr);

            // Initialize the vertex buffer view.
            m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
            m_VertexBufferView.StrideInBytes = sizeof(Vertex);
            m_VertexBufferView.SizeInBytes = vertexBufferSize;
        }

        // Create synchronization objects and wait until assets have been uploaded to the GPU.
        {
            D3D12Tests::ThrowIfFailed(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)));
            m_FenceValue = 1;

            // Create an event handler to use for frame synchronization.
            m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            if (m_FenceEvent == nullptr) {
                D3D12Tests::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
            }

            // Wait for the command list to execute; we are reusing the same command
            // list in our main loop but for now, we just want to wait for setup to
            // complete before continuing.
            WaitForPreviousFrame();
        }
    }

    void HelloTriangle::PopulateCommandList() {
        // Command list allocators can only be reset when the associated
        // command lists have finished executing on the GPU; apps should use
        // fences to determine GPU execution progress.
        D3D12Tests::ThrowIfFailed(m_CommandAllocator->Reset());

        // However, when ExecuteCommandList() is called on a particular command
        // list, the command list can then be reset at any time and must be before
        // re-recording.
        D3D12Tests::ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), m_PipelineState.Get()));

        // Set necessary states.
        m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
        m_CommandList->RSSetViewports(1, &m_Viewport);
        m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

        // Indicate that the back buffer will be used as a render target.
        auto transition = CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTargets[m_FrameIndex].Get(),
                                                               D3D12_RESOURCE_STATE_PRESENT,
                                                               D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_CommandList->ResourceBarrier(1, &transition);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_RtvHeap->GetCPUDescriptorHandleForHeapStart(), m_FrameIndex,
                                                m_RtvDescriptorSize);
        m_CommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        // Record commands
        constexpr D3D12Tests::Float32 clearColor[] = {0.0f, 0.2f, 0.4f, 1.0f};
        m_CommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_CommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
        m_CommandList->DrawInstanced(3, 1, 0, 0);

        // Indicate that the back buffer will now be used to present.
        transition = CD3DX12_RESOURCE_BARRIER::Transition(m_RenderTargets[m_FrameIndex].Get(),
                                                          D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                          D3D12_RESOURCE_STATE_PRESENT);
        m_CommandList->ResourceBarrier(1, &transition);

        D3D12Tests::ThrowIfFailed(m_CommandList->Close());
    }

    void HelloTriangle::WaitForPreviousFrame() {
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
