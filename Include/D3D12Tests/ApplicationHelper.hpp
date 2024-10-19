// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of D3D12 Tests.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#ifndef D3D12TESTS_APPLICATIONHELPER_HPP
#define D3D12TESTS_APPLICATIONHELPER_HPP

#include "D3D12Tests/pch.hpp"

#include <stdexcept>

namespace D3D12Tests {
	// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
	// it has no understanding of the lifetime of resources on the GPU. Apps must account
	// for the GPU lifetime of resources to avoid destroying objects that may still be
	// referenced by the GPU.
	using Microsoft::WRL::ComPtr;

	inline std::string HrToString(HRESULT hr);

	class HrException : public std::runtime_error {
	public:
		inline explicit HrException(HRESULT hr);
		inline HRESULT Error() const;

	private:
		const HRESULT m_Hr;
	};

#define SAFE_RELEASE(p) if (p) (p)->Release()

	inline void ThrowIfFailed(HRESULT hr);

	inline void GetAssetsPath(_Out_writes_(pathSize) WCHAR* path, UINT pathSize);

	inline HRESULT ReadDataFromFile(LPCWSTR filename, byte** data, UINT* size);

	inline HRESULT ReadDataFromDDSFile(LPCWSTR filename, byte** data, UINT* offset, UINT* size);

	// Assign a name to the object to help debugging.
	inline void SetName(ID3D12Object* pObject, LPCWSTR name);

	inline void SetNameIndexed(ID3D12Object* pObject, LPCWSTR name, UINT index);

// Naming helper for ComPtr<T>.
// Assigns the name of the variable as the name of the object.
// The indexed variant will include the index in the name of the object.
#define NAME_D3D12_OBJECT(x) SetName((x).Get(), L#x)
#define NAME_D3D12_OBJECT_INDEXED(x, n)	SetNameIndexed((x)[n].Get(), L#x, n)

	inline UINT CalculateConstantBufferByteSize(UINT byteSize);

#ifdef D3D_COMPILE_STANDARD_FILE_INCLUDE
	inline Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entryPoint,
		const std::string& target
	);
#endif

	// Resets all elements in a ComPtr array.
	template <class T>
	void ResetComPtrArray(T* comPtrArray);


	// Resets all elements in a std::unique_ptr array.
	template <class T>
	void ResetComPtrArray(T* uniquePtrArray);
}

#include "D3D12Tests/ApplicationHelper.inl"

#endif // D3D12TESTS_APPLICATIONHELPER_HPP
