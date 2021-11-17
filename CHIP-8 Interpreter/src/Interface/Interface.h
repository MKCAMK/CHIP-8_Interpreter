#pragma once
#include "Interpreter\CHIP-8.h"
#include "Interface\Windows_include.h"

class CHIP_8_INTERFACE
{
private:
	CHIP_8* mInterpreter;
	HWND mWindow;
	HDC mInnerContext;
	HBRUSH mPixelUnset;
	HBRUSH mPixelSet;
	static const unsigned int mDEFAULT_IPS = 500;
	static const unsigned int mMAX_IPS = 2000;
	static const unsigned int mMIN_IPS = 100;
	unsigned int mNuberOfInstructionsPerSecond;
	LARGE_INTEGER mMainClockFrequency;
	LARGE_INTEGER mMainClockStart;
	LARGE_INTEGER mMainClockStop;
	LARGE_INTEGER mOne60thOfSecond;
	LARGE_INTEGER mTimerStart;
	LARGE_INTEGER mTimerStop;
	bool mSoundPlaying;
	bool mStartupMessage;
	bool mError;
	unsigned char* BeepWave;

	void Reset();
	void HandleError(CHIP_8_ERROR_CODE);
	void LoadProgram(wchar_t*);

public:
	CHIP_8_INTERFACE(HWND, HDC, HBRUSH, HBRUSH);
	~CHIP_8_INTERFACE();
	void DropFile(WPARAM);
	void PressButton(WPARAM);
	void UnpressButton(WPARAM);
	void StartSound();
	void StopSound();
	void UpdateSpeed();
	void IncreaseSpeed();
	void DecreaseSpeed();
	void Run();
};