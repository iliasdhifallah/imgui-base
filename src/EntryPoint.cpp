#include "../pch.h"

#include "Utils/Settings.h"
#include "Interface/UserInterface.h"

int main()
{
	ShowWindow(GetConsoleWindow(), SW_HIDE);

	std::thread([] {
		const auto userInterface = new UserInterface(850.f, 595.f);
		userInterface->Display();
		delete userInterface;
	}).detach();

	
	while (!Instance<Settings>::Get()->m_Destruct) {
		Sleep(1);
	}

	Sleep(3500);
	return 0;
}