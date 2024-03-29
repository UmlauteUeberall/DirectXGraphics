#pragma once

struct SWindowSettings
{
	const long m_WindowWidth = 1024;
	const long m_WindowHeigth = 768;
	const bool m_Fullscreen = false;

	LPCWSTR m_WindowClassName = L"Graphics Class";
	LPCSTR m_WindowClassNameShort = "Graphics Class";
	LPCSTR m_WindowName = "Willkommen einfach nur Willkommen";
	HWND m_WindowHandle = nullptr;

	const bool m_enableVSync = false;
};