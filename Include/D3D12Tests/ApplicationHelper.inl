// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of D3D12 Tests.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

namespace D3D12Tests {
    inline std::string HrToString(HRESULT hr) {
        char sStr[64] = {};
        sprintf_s(sStr, "HRESULT of 0x%08X", static_cast<UINT>(hr));
        return std::string(sStr);
    }

    inline HrException::HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_Hr(hr) {}

    inline HRESULT HrException::Error() const {
        return m_Hr;
    }

    inline void ThrowIfFailed(HRESULT hr) {
        if (FAILED(hr)) {
            throw HrException(hr);
        }
    }

    inline void GetAssetsPath(_Out_writes_(pathSize) WCHAR* path, UINT pathSize) {
        if (path == nullptr) {
            throw std::exception();
        }

        DWORD size = GetModuleFileName(nullptr, path, pathSize);
        if (size == 0 || size == pathSize) {
            // Method failed or path was truncated.
            throw std::exception();
        }

        WCHAR* lastSlash = wcsrchr(path, L'\\');
        if (lastSlash) {
            *(lastSlash + 1) = L'\0';
        }
    }
    
    inline HRESULT ReadDataFromFile(LPCWSTR filename, byte** data, UINT* size) {
        using namespace Microsoft::WRL;

#if WINVER >= _WIN32_WINNT_WIN8
        CREATEFILE2_EXTENDED_PARAMETERS extendedParams = {};
        extendedParams.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
        extendedParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        extendedParams.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
        extendedParams.dwSecurityQosFlags = SECURITY_ANONYMOUS;
        extendedParams.lpSecurityAttributes = nullptr;
        extendedParams.hTemplateFile = nullptr;

        Wrappers::FileHandle file(CreateFile2(filename, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extendedParams));
#else
        Wrappers::FileHandle file(CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN | SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS, nullptr));
#endif

        if (file.Get() == INVALID_HANDLE_VALUE) {
            throw std::exception();
        }

        FILE_STANDARD_INFO fileInfo = {};
        if (!GetFileInformationByHandleEx(file.Get(), FileStandardInfo, &fileInfo, sizeof(fileInfo))) {
            throw std::exception();
        }

        if (fileInfo.EndOfFile.HighPart != 0) {
            throw std::exception();
        }

        *data = reinterpret_cast<byte*>(malloc(fileInfo.EndOfFile.LowPart));
        *size = fileInfo.EndOfFile.LowPart;

        if (!ReadFile(file.Get(), *data, fileInfo.EndOfFile.LowPart, nullptr, nullptr)) {
            throw std::exception();
        }

        return S_OK;
    }

    inline HRESULT ReadDataFromDDSFile(LPCWSTR filename, byte** data, UINT* offset, UINT* size) {
        if (FAILED(ReadDataFromFile(filename, data, size))) {
            return E_FAIL;
        }

        // DDS files always start with the same magic number.
        static const UINT DDS_MAGIC = 0x20534444;
        UINT magicNumber = *reinterpret_cast<const UINT*>(*data);
        if (magicNumber != DDS_MAGIC) {
            return E_ABORT;
        }

        struct DDS_PIXELFORMAT {
            UINT Size;
            UINT Flags;
            UINT FourCC;
            UINT RgbBitCount;
            UINT RBitMask;
            UINT GBitMask;
            UINT BBitMask;
            UINT ABitMask;
        };

        struct DDS_HEADER {
            UINT Size;
            UINT Flags;
            UINT Height;
            UINT Width;
            UINT PitchOrLinearDepth;
            UINT Depth;
            UINT MipMapCount;
            UINT Reserved1[11];
            DDS_PIXELFORMAT DdsPixelFormat;
            UINT Caps;
            UINT Caps2;
            UINT Caps3;
            UINT Caps4;
            UINT Reserved;
        };

        auto ddsHeader = reinterpret_cast<const DDS_HEADER*>(*data + sizeof(UINT));
        if (ddsHeader->Size != sizeof(DDS_HEADER) || ddsHeader->DdsPixelFormat.Size != sizeof(DDS_PIXELFORMAT)) {
            return E_FAIL;
        }

        const ptrdiff_t ddsDataOffset = sizeof(UINT) + sizeof(DDS_HEADER);
        *offset = ddsDataOffset;
        *size = *size - ddsDataOffset;

        return S_OK;
    }

#ifdef D3D12TESTS_DEBUG
    inline void SetName(ID3D12Object* pObject, LPCWSTR name) {
        pObject->SetName(name);
    }

    inline void SetNameIndexed(ID3D12Object* pObject, LPCWSTR name, UINT index) {
        WCHAR fullName[50];
        if (swprintf_s(fullName, L"%s[%u]", name, index) > 0) {
            pObject->SetName(fullName);
        }
    }
#else
    inline void SetName(ID3D12Object* pObject, LPCWSTR name) {}

    inline void SetNameIndexed(ID3D12Object* pObject, LPCWSTR name, UINT index) {}
#endif

    inline UINT CalculateConstantBufferByteSize(UINT byteSize) {
        // Constant buffer size is required to be aligned.
        return (byteSize + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1));
    }

#ifdef D3D_COMPILE_STANDARD_FILE_INCLUDE
    inline Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
        const std::wstring& filename,
        const D3D_SHADER_MACRO* defines,
        const std::string& entryPoint,
        const std::string& target
    ) {
        UINT compileFlags = 0;
#ifdef D3D12TESTS_DEBUG
        compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        HRESULT hr;

        Microsoft::WRL::ComPtr<ID3DBlob> byteCode = nullptr;
        Microsoft::WRL::ComPtr<ID3DBlob> errors;
        hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

        if (errors != nullptr) {
            OutputDebugStringA((char*)errors->GetBufferPointer());
        }
        ThrowIfFailed(hr);

        return byteCode;
    }
#endif

    template <class T>
    void ResetComPtrArray(T* comPtrArray) {
        for (auto& i : *comPtrArray) {
            i.Reset();
        }
    }

    template <class T>
    void ResetComPtrArray(T* uniquePtrArray) {
        for (auto& i : *uniquePtrArray) {
            i.reset();
        }
    }
}
