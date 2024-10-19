// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of D3D12 Tests.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "HelloTexture/HelloTexture.hpp"

namespace HelloTexture {
    HelloTexture::HelloTexture(const UINT width, const UINT height, const std::wstring& name) :
        D3D12Tests::Application(width, height, name),
        m_Viewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)),
        m_ScissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
        m_RtvDescriptorSize(0),
        m_FrameIndex(0) {
    }

    void HelloTexture::OnInit() {
        LoadPipeline();
        LoadAssets();
    }

    void HelloTexture::OnUpdate() {
    }

    void HelloTexture::OnRender() {
        // Record all the commands we need to render the scene into the command list.
        PopulateCommandList();

        // Execute the command list.
        ID3D12CommandList* ppCommandLists[] = {m_CommandList.Get()};
        m_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // Present the frame.
        D3D12Tests::ThrowIfFailed(m_SwapChain->Present(1, 0));

        WaitForPreviousFrame();
    }

    void HelloTexture::OnDestroy() {
        WaitForPreviousFrame();

        CloseHandle(m_FenceEvent);
    }

    void HelloTexture::LoadPipeline() {
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

            // Describe and create a shader resource view (SRV) heap for the texture.
            D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
            srvHeapDesc.NumDescriptors = 1;
            srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            D3D12Tests::ThrowIfFailed(m_Device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SrvHeap)));

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
    void HelloTexture::LoadAssets() {
        // Create the root signature.
        {
            D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

            // This is the highest version the test supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
            featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

            if (FAILED(
                m_Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))) {
                featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
            }

            CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
            ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

            CD3DX12_ROOT_PARAMETER1 rootParameters[1];
            rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

            D3D12_STATIC_SAMPLER_DESC sampler = {};
            sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
            sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
            sampler.MipLODBias = 0;
            sampler.MaxAnisotropy = 0;
            sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
            sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            sampler.MinLOD = 0.0f;
            sampler.MaxLOD = D3D12_FLOAT32_MAX;
            sampler.ShaderRegister = 0;
            sampler.RegisterSpace = 0;
            sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
            rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler,
                                       D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

            ComPtr<ID3DBlob> signature;
            ComPtr<ID3DBlob> error;
            D3D12Tests::ThrowIfFailed(
                D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature,
                                                      &error));
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
                GetAssetFullPath(L"HelloTexture/Resources/Shaders/shader.hlsl").c_str(), nullptr, nullptr, "VSMain",
                "vs_5_0", compilerFlags, 0, &vertexShader, nullptr));
            D3D12Tests::ThrowIfFailed(D3DCompileFromFile(
                GetAssetFullPath(L"HelloTexture/Resources/Shaders/shader.hlsl").c_str(), nullptr, nullptr, "PSMain",
                "ps_5_0", compilerFlags, 0, &pixelShader, nullptr));

            // Define the vertex input layout
            D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
                {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
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

        // Create the vertex buffer.
        {
            // Define the geometry for a triangle.
            Vertex triangleVertices[] = {
                {{0.0f, 0.25f * m_AspectRatio, 0.0f}, {0.5f, 0.0f}},
                {{0.25f, -0.25f * m_AspectRatio, 0.0f}, {1.0f, 1.0f}},
                {{-0.25f, -0.25f * m_AspectRatio, 0.0f}, {0.0f, 1.0f}}
            };

            constexpr UINT vertexBufferSize = sizeof(triangleVertices);

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

        // Note: ComPtr's are CPU objects but this resource needs to stay in scope until
        // the command list that references it has finished executing on the GPU.
        // We will flush the GPU at the end of this method to ensure the resource is not
        // prematurely destroyed.
        ComPtr<ID3D12Resource> textureUploadHeap;

        // Create the texture
        {
            // Describe and create a texture2D
            D3D12_RESOURCE_DESC textureDesc = {};
            textureDesc.MipLevels = 1;
            textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            textureDesc.Width = TextureWidth;
            textureDesc.Height = TextureHeight;
            textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
            textureDesc.DepthOrArraySize = 1;
            textureDesc.SampleDesc.Count = 1;
            textureDesc.SampleDesc.Quality = 0;
            textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

            auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

            D3D12Tests::ThrowIfFailed(m_Device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &textureDesc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(&m_Texture)));

            const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_Texture.Get(), 0, 1);

            auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

            heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            // Create the GPU upload buffer.
            D3D12Tests::ThrowIfFailed(m_Device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&textureUploadHeap)));

            // Copy data to the intermediate upload heap and then schedule a copy
            // from the upload heap to the Texture2D.
            std::vector<UINT8> texture = GenerateTextureData();

            D3D12_SUBRESOURCE_DATA textureData = {};
            textureData.pData = &texture[0];
            textureData.RowPitch = TextureWidth * TexturePixelSize;
            textureData.SlicePitch = textureData.RowPitch * TextureHeight;

            UpdateSubresources(m_CommandList.Get(), m_Texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData);
            auto transition = CD3DX12_RESOURCE_BARRIER::Transition(m_Texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                                   D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            m_CommandList->ResourceBarrier(1, &transition);

            // Describe and create an SRV for the texture.
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = textureDesc.Format;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
            m_Device->CreateShaderResourceView(m_Texture.Get(), &srvDesc,
                                               m_SrvHeap->GetCPUDescriptorHandleForHeapStart());
        }

        // Close the command list and execute it to begin the initial GPU setup.
        D3D12Tests::ThrowIfFailed(m_CommandList->Close());
        ID3D12CommandList* ppCommandLists[] = {m_CommandList.Get()};
        m_CommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

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

    std::vector<UINT8> HelloTexture::GenerateTextureData() {
        constexpr UINT rowPitch = TextureWidth * TexturePixelSize;
        constexpr UINT cellPitch = rowPitch >> 3; // The width of a cell in the checkerboard texture.
        constexpr UINT cellHeight = TextureWidth >> 3; // The height of a cell in the checkerboard texture.
        constexpr UINT textureSize = rowPitch * TextureHeight;

        std::vector<UINT8> data(textureSize);
        UINT8* pData = &data[0];

        for (UINT n = 0; n < textureSize; n += TexturePixelSize) {
            UINT x = n % rowPitch;
            UINT y = n / rowPitch;
            UINT i = x / cellPitch;
            UINT j = y / cellHeight;

            if (i % 2 == j % 2) {
                pData[n] = 0x00; // R
                pData[n + 1] = 0x00; // G
                pData[n + 2] = 0x00; // B
                pData[n + 3] = 0xff; // A
            } else {
                pData[n] = 0xff; // R
                pData[n + 1] = 0xff; // G
                pData[n + 2] = 0xff; // B
                pData[n + 3] = 0xff; // A
            }
        }

        return data;
    }

    void HelloTexture::PopulateCommandList() {
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

        ID3D12DescriptorHeap* ppHeaps[] = {m_SrvHeap.Get()};
        m_CommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        m_CommandList->SetGraphicsRootDescriptorTable(0, m_SrvHeap->GetGPUDescriptorHandleForHeapStart());
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

    void HelloTexture::WaitForPreviousFrame() {
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
