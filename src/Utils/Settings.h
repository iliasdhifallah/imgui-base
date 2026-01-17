#pragma once
#include <vector>

class Settings {
public:
	bool m_Destruct{ false };
};

class Globals
{
public:
	HWND m_Hwnd;
	DWORD m_Pid;

	struct
	{
		bool m_Rainbow{ false };
		float m_AccentColor[3]{ 59.f / 255.f, 120.f / 255.f, 254.f / 255.f };
	}Gui;
};