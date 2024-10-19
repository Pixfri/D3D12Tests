// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of D3D12 Tests.
// For conditions of distribution and use, see copyright notice in LICENSE.

#pragma once

#ifndef D3D12TESTS_PCH_HPP
#define D3D12TESTS_PCH_HPP

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN // Exclude unnecessary stuff from Windows.h
#endif // WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <directx/d3dx12.h>

#include <string>

#include <wrl.h>
#include <shellapi.h>

namespace D3D12Tests {
	using Int8 = int8_t;
	using Int16 = int16_t;
	using Int32 = int32_t;
	using Int64 = int64_t;

	using UInt8 = uint8_t;
	using UInt16 = uint16_t;
	using UInt32 = uint32_t;
	using UInt64 = uint64_t;

	using Float32 = float;
	using Float64 = double;

	static_assert(sizeof(Int8) == 1, "Int8 is supposed to be 1 byte long.");
	static_assert(sizeof(Int16) == 2, "Int16 is supposed to be 2 bytes long.");
	static_assert(sizeof(Int32) == 4, "Int32 is supposed to be 4 bytes long.");
	static_assert(sizeof(Int64) == 8, "Int64 is supposed to be 8 bytes long.");

	static_assert(sizeof(UInt8) == 1, "UInt8 is supposed to be 1 byte long.");
	static_assert(sizeof(UInt16) == 2, "UInt16 is supposed to be 2 bytes long.");
	static_assert(sizeof(UInt32) == 4, "UInt32 is supposed to be 4 bytes long.");
	static_assert(sizeof(UInt64) == 8, "UInt64 is supposed to be 8 bytes long.");

	static_assert(sizeof(Float32) == 4, "Float32 is supposed to be 4 bytes long.");
	static_assert(sizeof(Float64) == 8, "Float64 is supposed to be 8 bytes long.");
}

#endif // D3D12TESTS_PCH_HPP
