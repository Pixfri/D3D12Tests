// Copyright (C) 2024 Jean "Pixfri" Letessier 
// This file is part of D3D12 Tests.
// For conditions of distribution and use, see copyright notice in LICENSE.

#include "Framework/Win32ApplicationBase.hpp"

namespace D3D12Tests {
	HWND Win32ApplicationBase::m_HWnd = nullptr;

	Int32 Win32ApplicationBase::Run(Application* pApplication, HINSTANCE hInstance, Int32 nCmdShow) {
		// Parse the command line parameters.
		Int32 argc;
		LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
		pApplication->ParseCommandLineArgs(argv, argc);
		LocalFree(argv);

		// Initialize the window class.
		WNDCLASSEX windowClass = { 0 };
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = WindowProc;
		windowClass.hInstance = hInstance;
		windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		windowClass.lpszClassName = L"D3D12TestsClass";
		RegisterClassEx(&windowClass);

		RECT windowRect = { 0, 0, static_cast<LONG>(pApplication->GetWidth()), static_cast<LONG>(pApplication->GetHeight()) };
		AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

		// Create the window and store the handle to it.
		m_HWnd = CreateWindow(
			windowClass.lpszClassName,
			pApplication->GetTitle(),
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			nullptr, // No parent window
			nullptr, // No menu
			hInstance,
			pApplication
		);

		// Initialize the application. OnInit is defined by each child of Application.
		pApplication->OnInit();

		ShowWindow(m_HWnd, nCmdShow);

		// Main application loop.
		MSG msg = {};
		while (msg.message != WM_QUIT) {
			// Process any messages in the queue.
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		pApplication->OnDestroy();

		// Return this part of the WM_QUIT message to windows.
		return static_cast<char>(msg.wParam);
	}

	LRESULT CALLBACK Win32ApplicationBase::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
		Application* application = reinterpret_cast<Application*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

		switch (message) {
		case WM_CREATE:
			{
				// Save the Application* passed to CreateWindow.
				LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
				SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
			}
			return 0;

		case WM_KEYDOWN:
			if (application) {
				application->OnKeyDown(static_cast<UINT8>(wParam));
			}
			return 0;

		case WM_KEYUP:
			if (application) {
				application->OnKeyUp(static_cast<UINT8>(wParam));
			}
			return 0;

		case WM_PAINT:
			if (application) {
				application->OnUpdate();
				application->OnRender();
			}
			return 0;

		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}

		// Handle any messages the switch statement didn't.
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}
