#include "Interface\Interface.h"
#include "resources\resource.h"

#include <Mmsystem.h>
#pragma comment(lib, "winmm.lib")
#include <shellapi.h>
#include <string>
#include <stdexcept>

void CHIP_8_INTERFACE::Reset()
{
	mMainClockStart.QuadPart = 0;;
	mMainClockStop.QuadPart = 0;;
	mTimerStart.QuadPart = 0;
	mTimerStop.QuadPart = 0;
	StopSound();
	mError = false;
}

void CHIP_8_INTERFACE::HandleError(CHIP_8_ERROR_CODE Error)
{
	if (!mError)
	{
		switch (Error)
		{
			case CHIP_8_ERROR_CODE__RESET:
			{
				if (!mStartupMessage)
				{
					std::wstring Text = L"This is a CHIP-8 language interpreter.\nDrag and drop a program file to load.\nUse the + and - keys to change Instructions Per Second.\n\n";
#if INCORRECT_SHIFT_INSTRUCTIONS_VERSION == true
					Text += L"This version of the interpreter implements the binary shift instructions according to changed, popular definitions. Programs expecting the original definitions may not work correctly.\n\n ";
#else
					Text += L"This version of the interpreter implements the binary shift instructions according to the original definitions. Programs expecting later, changed definitions may not work correctly.\n\n ";
#endif
#if INCORRECT_MEMORY_INSTRUCTIONS_VERSION == true
					Text += L"This version of the interpreter implements the memory store and load instructions according to changed, popular definitions. Programs expecting the original definitions may not work correctly.\n\n ";
#else
					Text += L"This version of the interpreter implements the memory store and load instructions according to the original definitions. Programs expecting later, changed definitions may not work correctly.\n\n ";
#endif
					Text += L"Key bindings:\n0 :  0\n1 :  1\n2 :  2 ,  Up Arrow\n3 :  3\n4 :  4 ,  Left Arrow\n5 :  5 ,  Space Bar\n6 :  6 ,  Right Arrow\n7 :  7\n8 :  8\n9 :  9\nA :  A\nB :  B ,  Down Arrow\nC :  C\nD :  D\nE :  E\nF :  F";
					MessageBox(mWindow, Text.c_str(), L"CHIP-8 Interpreter", 0);
					mStartupMessage = true;
				}
				break;
			}
			case CHIP_8_ERROR_CODE__PROGRAM_TOO_BIG:
			{
				MessageBox(mWindow, L"Program is too big to fit in the memory.", L"Error has occurred", 0);
				mError = true;
				break;
			}
			case CHIP_8_ERROR_CODE__OUT_OF_BOUNDS_MEMORY_ACCESS:
			{
				MessageBox(mWindow, L"Memory was accessed out of bounds.", L"Error has occurred", 0);
				mError = true;
				break;
			}
			case CHIP_8_ERROR_CODE__INSTRUCTION_NOT_RECOGNIZED:
			{
				MessageBox(mWindow, L"Instruction was not recognized.", L"Error has occurred", 0);
				mError = true;
				break;
			}
			case CHIP_8_ERROR_CODE__INSTRUCTION_0NNN_NOT_IMPLEMENTED:
			{
				MessageBox(mWindow, L"Instruction 0NNN is not implemented.", L"Error has occurred", 0);
				mError = true;
				break;
			}
			case CHIP_8_ERROR_CODE__STACK_OVERFLOW:
			{
				MessageBox(mWindow, L"Stack overflowed.", L"Error has occurred", 0);
				mError = true;
				break;
			}
			case CHIP_8_ERROR_CODE__STACK_UNDERFLOW:
			{
				MessageBox(mWindow, L"Stack underflowed.", L"Error has occurred", 0);
				mError = true;
				break;
			}
		}
	}
}

void CHIP_8_INTERFACE::LoadProgram(wchar_t* Filename)
{
	Reset();

	HANDLE File;
	if ((File = CreateFile(Filename, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0)) != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER Size;
		GetFileSizeEx(File, &Size);
		LPVOID Buffer = new char[Size.LowPart];
		if (ReadFile(File, Buffer, Size.LowPart, 0, 0))
			mInterpreter->LoadProgram((char*)Buffer, static_cast<unsigned int>(Size.LowPart));
		delete[] Buffer;
		CloseHandle(File);
	}

	QueryPerformanceCounter(&mMainClockStart);
	QueryPerformanceCounter(&mTimerStart);
}

bool LoadBeepWave(unsigned char*& BeepWave)
{
	HMODULE ResourceModule = GetModuleHandle(NULL);

	HRSRC ResourceHandle = FindResource(ResourceModule, MAKEINTRESOURCE(IDR_WAVE1), L"WAVE");
	if (ResourceHandle == NULL)
		return false;

	HGLOBAL ResourceMemory = LoadResource(ResourceModule, ResourceHandle);
	if (ResourceMemory == NULL)
		return false;

	DWORD ResourceSize = SizeofResource(ResourceModule, ResourceHandle);
	if (ResourceSize == 0)
		return false;

	LPVOID ResourcePointer = LockResource(ResourceMemory);
	if (ResourcePointer == NULL)
		return false;

	BeepWave = static_cast<unsigned char*>(ResourcePointer);

	return true;
}

CHIP_8_INTERFACE::CHIP_8_INTERFACE(HWND Window, HDC InnerContext, HBRUSH PixelUnset, HBRUSH PixelSet) : mWindow{ Window }, mInnerContext{ InnerContext }, mPixelUnset{ PixelUnset }, mPixelSet{ PixelSet }
{
	mInterpreter = new CHIP_8;

	mNuberOfInstructionsPerSecond = mDEFAULT_IPS;
	UpdateSpeed();

	QueryPerformanceFrequency(&mOne60thOfSecond);
	mOne60thOfSecond.QuadPart /= 60;

	if (!LoadBeepWave(BeepWave))
		throw(std::runtime_error("Could not find resource."));

	Reset();
	mStartupMessage = false;
}

CHIP_8_INTERFACE::~CHIP_8_INTERFACE()
{
	delete mInterpreter;
}

void CHIP_8_INTERFACE::DropFile(WPARAM hDrop)
{
	if (1 == DragQueryFile((HDROP)hDrop, 0xFFFFFFFF, 0, 0))
	{
		unsigned int FilenameLength = DragQueryFile((HDROP)hDrop, 0, 0, 0) + 1;
		wchar_t* Filename = new wchar_t[FilenameLength];
		if ((DragQueryFile((HDROP)hDrop, 0, Filename, FilenameLength) + 1) == FilenameLength)
			LoadProgram(Filename);
		delete[] Filename;
	}

	DragFinish((HDROP)hDrop);
}

void CHIP_8_INTERFACE::PressButton(WPARAM Button)
{
	switch (Button)
	{
		case VK_NUMPAD0:
		case 0x30:
			mInterpreter->PressButton(0x0);
			break;
		case VK_NUMPAD1:
		case 0x31:
			mInterpreter->PressButton(0x1);
			break;
		case VK_NUMPAD2:
		case 0x32:
		case VK_UP:
			mInterpreter->PressButton(0x2);
			break;
		case VK_NUMPAD3:
		case 0x33:
			mInterpreter->PressButton(0x3);
			break;
		case VK_NUMPAD4:
		case 0x34:
		case VK_LEFT:
			mInterpreter->PressButton(0x4);
			break;
		case VK_NUMPAD5:
		case 0x35:
		case VK_SPACE:
			mInterpreter->PressButton(0x5);
			break;
		case VK_NUMPAD6:
		case 0x36:
		case VK_RIGHT:
			mInterpreter->PressButton(0x6);
			break;
		case VK_NUMPAD7:
		case 0x37:
			mInterpreter->PressButton(0x7);
			break;
		case VK_NUMPAD8:
		case 0x38:
		case VK_DOWN:
			mInterpreter->PressButton(0x8);
			break;
		case VK_NUMPAD9:
		case 0x39:
			mInterpreter->PressButton(0x9);
			break;
		case 'A':
			mInterpreter->PressButton(0xA);
			break;
		case 'B':
			mInterpreter->PressButton(0xB);
			break;
		case 'C':
			mInterpreter->PressButton(0xC);
			break;
		case 'D':
			mInterpreter->PressButton(0xD);
			break;
		case 'E':
			mInterpreter->PressButton(0xE);
			break;
		case 'F':
			mInterpreter->PressButton(0xF);
			break;
		case VK_ESCAPE:
			SendMessage(mWindow, WM_CLOSE, 0, 0);
			break;
		case VK_ADD:
		case VK_OEM_PLUS:
			IncreaseSpeed();
			break;
		case VK_SUBTRACT:
		case VK_OEM_MINUS:
			DecreaseSpeed();
			break;
	}
}

void CHIP_8_INTERFACE::UnpressButton(WPARAM Button)
{
	switch (Button)
	{
		case VK_NUMPAD0:
		case 0x30:
			mInterpreter->UnpressButton(0x0);
			break;
		case VK_NUMPAD1:
		case 0x31:
			mInterpreter->UnpressButton(0x1);
			break;
		case VK_NUMPAD2:
		case 0x32:
		case VK_UP:
			mInterpreter->UnpressButton(0x2);
			break;
		case VK_NUMPAD3:
		case 0x33:
			mInterpreter->UnpressButton(0x3);
			break;
		case VK_NUMPAD4:
		case 0x34:
		case VK_LEFT:
			mInterpreter->UnpressButton(0x4);
			break;
		case VK_NUMPAD5:
		case 0x35:
		case VK_SPACE:
			mInterpreter->UnpressButton(0x5);
			break;
		case VK_NUMPAD6:
		case 0x36:
		case VK_RIGHT:
			mInterpreter->UnpressButton(0x6);
			break;
		case VK_NUMPAD7:
		case 0x37:
			mInterpreter->UnpressButton(0x7);
			break;
		case VK_NUMPAD8:
		case 0x38:
		case VK_DOWN:
			mInterpreter->UnpressButton(0x8);
			break;
		case VK_NUMPAD9:
		case 0x39:
			mInterpreter->UnpressButton(0x9);
			break;
		case 'A':
			mInterpreter->UnpressButton(0xA);
			break;
		case 'B':
			mInterpreter->UnpressButton(0xB);
			break;
		case 'C':
			mInterpreter->UnpressButton(0xC);
			break;
		case 'D':
			mInterpreter->UnpressButton(0xD);
			break;
		case 'E':
			mInterpreter->UnpressButton(0xE);
			break;
		case 'F':
			mInterpreter->UnpressButton(0xF);
			break;
	}
}

void CHIP_8_INTERFACE::StartSound()
{
	PlaySound((LPWSTR)BeepWave, NULL, SND_MEMORY | SND_ASYNC | SND_LOOP);
	mSoundPlaying = true;
}

void CHIP_8_INTERFACE::StopSound()
{
	PlaySound(0, 0, 0);
	mSoundPlaying = false;
}

void CHIP_8_INTERFACE::UpdateSpeed()
{
	QueryPerformanceFrequency(&mMainClockFrequency);
	mMainClockFrequency.QuadPart /= mNuberOfInstructionsPerSecond;
	std::wstring Caption = L"CHIP-8 Interpreter   IPS: " + std::to_wstring(mNuberOfInstructionsPerSecond);
	SetWindowText(mWindow, Caption.c_str());
}

void CHIP_8_INTERFACE::IncreaseSpeed()
{
	if (mNuberOfInstructionsPerSecond < mMAX_IPS)
	{
		mNuberOfInstructionsPerSecond += 100;
		UpdateSpeed();
	}
}

void CHIP_8_INTERFACE::DecreaseSpeed()
{
	if (mNuberOfInstructionsPerSecond > mMIN_IPS)
	{
		mNuberOfInstructionsPerSecond -= 100;
		UpdateSpeed();
	}
}

void CHIP_8_INTERFACE::Run()
{
	QueryPerformanceCounter(&mMainClockStop);
	if (mMainClockStop.QuadPart - mMainClockStart.QuadPart < mMainClockFrequency.QuadPart)
		return;
	mMainClockStart.QuadPart = mMainClockStop.QuadPart;

	QueryPerformanceCounter(&mTimerStop);
	LARGE_INTEGER TimerDelta;
	TimerDelta.QuadPart = mTimerStop.QuadPart - mTimerStart.QuadPart;
	TimerDelta.QuadPart /= mOne60thOfSecond.QuadPart;

	CHIP_8_ERROR_CODE Result;
	if (Result = mInterpreter->Step(static_cast <unsigned int>(TimerDelta.QuadPart)))
		HandleError(Result);

	if (TimerDelta.QuadPart > 0)
		mTimerStart.QuadPart = mTimerStop.QuadPart;

	if (mInterpreter->GetSound())
	{
		if (!mSoundPlaying)
		{
			StartSound();
		}
	}
	else
	{
		if (mSoundPlaying)
		{
			StopSound();
		}
	}

	if (mInterpreter->DidDrawingHappen())
	{
		RECT Rect = { 0, 0, mInterpreter->RESOLUTION_X, mInterpreter->RESOLUTION_Y };
		FillRect(mInnerContext, &Rect, mPixelUnset);

		bool Drawing = false;
		for (unsigned int y = 0; y < mInterpreter->RESOLUTION_Y; ++y)
		{
			for (unsigned int x = 0; x <= mInterpreter->RESOLUTION_X; ++x)
			{
				if (x == mInterpreter->RESOLUTION_X)
				{
					if (Drawing)
					{
						Rect.right = x;
						Rect.bottom = Rect.top + 1;
						FillRect(mInnerContext, &Rect, mPixelSet);
						Drawing = false;
					}
				}
				else
				{
					if (Drawing)
					{
						if (!mInterpreter->GetDisplay(x, y))
						{
							Rect.right = x;
							Rect.bottom = Rect.top + 1;
							FillRect(mInnerContext, &Rect, mPixelSet);
							Drawing = false;
						}
					}
					else
					{
						if (mInterpreter->GetDisplay(x, y))
						{
							Rect.left = x;
							Rect.top = y;
							Drawing = true;
						}
					}
				}
			}
		}
	}
}