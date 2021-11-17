#include "resources\resource.h"
#include "Interpreter\CHIP-8.h"
#include "Interface\Interface.h"
#include "Interface\Windows_include.h"

#include <shellapi.h>

CHIP_8_INTERFACE* Interface = nullptr;

const int RefreshRate = 30;

const RECT InnerContextSize = { 0, 0, CHIP_8::RESOLUTION_X, CHIP_8::RESOLUTION_Y };
HDC InnerContext = NULL;
HBITMAP InnerContext_Bitmap = NULL;

HBRUSH PixelUnset = CreateSolidBrush(RGB(0, 0, 0));
HBRUSH PixelSet = CreateSolidBrush(RGB(255, 255, 255));

LRESULT WINAPI EventProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
		case WM_CREATE:
		{
			HDC hdcWindowHandle = GetDC(hWnd);
			InnerContext = CreateCompatibleDC(hdcWindowHandle);
			ReleaseDC(hWnd, hdcWindowHandle);
			SaveDC(InnerContext);
			InnerContext_Bitmap = CreateCompatibleBitmap(InnerContext, InnerContextSize.right, InnerContextSize.bottom);
			if (SelectObject(InnerContext, InnerContext_Bitmap) == NULL)
			{
				DestroyWindow(hWnd);
				break;
			}

			Interface = new CHIP_8_INTERFACE(hWnd, InnerContext, PixelUnset, PixelSet);
			break;
		}
		case WM_DESTROY:
		{
			delete Interface;
			Interface = nullptr;

			RestoreDC(InnerContext, -1);
			DeleteDC(InnerContext);

			DeleteObject(InnerContext_Bitmap);

			PostQuitMessage(0);
			return 0;
		}
		case WM_DROPFILES:
		{
			Interface->DropFile(wParam);
			break;
		}
		case WM_PAINT:
		{
			RECT Rect;
			GetClientRect(hWnd, &Rect);
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			StretchBlt(hdc, 0, 0, Rect.right, Rect.bottom, InnerContext, 0, 0, InnerContextSize.right, InnerContextSize.bottom, SRCCOPY);
			EndPaint(hWnd, &ps);
			break;
		}
		case WM_KEYDOWN:
		{
			Interface->PressButton(wParam);
			break;
		}
		case WM_KEYUP:
		{
			Interface->UnpressButton(wParam);
			break;
		}
	}
	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nShowCmd)
{
	LARGE_INTEGER Frequency;
	if (!QueryPerformanceFrequency(&Frequency)) return 0;
	LARGE_INTEGER Tick, Start, Stop;
	Tick.QuadPart = Frequency.QuadPart / RefreshRate;

	HWND WindowHandle;
	WNDCLASSEX WindowClass;
	TCHAR WindowClassName[] = L"CHIP_8_2020.01.11";

	ZeroMemory(&WindowClass, sizeof(WNDCLASSEX));
	WindowClass.hInstance = hInstance;
	WindowClass.lpszClassName = WindowClassName;
	WindowClass.lpfnWndProc = EventProc;
	WindowClass.cbSize = sizeof(WNDCLASSEX);
	WindowClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WindowClass.hbrBackground = PixelUnset;
	WindowClass.style = CS_HREDRAW | CS_VREDRAW;

	if (!RegisterClassEx(&WindowClass)) return 0;

	RECT WindowRect = {0, 0, CHIP_8::RESOLUTION_X * 10, CHIP_8::RESOLUTION_Y * 10};
	if (0 == AdjustWindowRectEx(&WindowRect, WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, true, WS_EX_ACCEPTFILES))
	{
		WindowRect.left = CW_USEDEFAULT;
		WindowRect.top = CW_USEDEFAULT;
		WindowRect.right = CW_USEDEFAULT;
		WindowRect.bottom = CW_USEDEFAULT;
	}
	else
	{
		WindowRect.right -= WindowRect.left;
		WindowRect.bottom -= WindowRect.top;
		WindowRect.left = 0;
		WindowRect.top = 0;
	}

	WindowHandle = CreateWindowEx(WS_EX_ACCEPTFILES, WindowClassName, L"", WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, WindowRect.left, WindowRect.top, WindowRect.right, WindowRect.bottom, NULL, NULL, hInstance, NULL);

	ShowWindow(WindowHandle, nShowCmd);

	MSG Msg;

	QueryPerformanceCounter(&Start);
	for (;;)
	{
		if (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
		{
			if (Msg.message == WM_QUIT) break;
			DispatchMessage(&Msg);
		}
		if (Interface)
			Interface->Run();
		QueryPerformanceCounter(&Stop);
		if (Stop.QuadPart - Start.QuadPart >= Tick.QuadPart)
		{
			InvalidateRect(WindowHandle, NULL, false);
			Start = Stop;
		}
	}

	DeleteObject(PixelUnset);
	DeleteObject(PixelSet);

	return static_cast<int>(Msg.wParam);
}