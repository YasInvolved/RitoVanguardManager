#pragma once
#include <Windows.h>
#include <string>

#include "NonCopyable.h"
#include "WinAPIException.h"

#ifndef NDEBUG
#include <iostream>
#endif

#define CREATE_NID_TEMPLATE { .cbSize = sizeof(NOTIFYICONDATA), .hWnd = hWnd, .uID = ID_TRAY, .uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO, .uCallbackMessage = WM_USER, .hIcon = LoadIcon(NULL, IDI_APPLICATION),.dwInfoFlags = NIIF_USER }

constexpr uint32_t ID_TRAY = 1;
constexpr wchar_t wndClassname[] = L"vgm_tray_dummy_window";

namespace VanguardManager {
	class TrayIcon : NonCopyable {
	private:
		HINSTANCE hInstance;
		HWND hWnd;
		WNDCLASSEX wc = { };

	public:
		TrayIcon() {
			ZeroMemory(&wc, sizeof(WNDCLASSEX));
			wc.cbSize = sizeof(WNDCLASSEX);
			wc.style = 0;
			wc.lpfnWndProc = WndProc;
			wc.cbClsExtra = 0;
			wc.cbWndExtra = 0;
			wc.hInstance = hInstance;
			wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
			wc.hCursor = LoadCursor(NULL, IDC_ARROW);
			wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
			wc.lpszMenuName = NULL;
			wc.lpszClassName = wndClassname;
			wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
			hInstance = GetModuleHandle(NULL);

			if (!RegisterClassExW(&wc)) throw std::runtime_error("Failed to register dummy window class");
			hWnd = CreateWindowEx(
				WS_EX_WINDOWEDGE,
				wndClassname,
				L"vgm dummy window",
				WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT,
				NULL,
				NULL,
				hInstance,
				NULL
			);

			#ifndef NDEBUG
			std::cout << "Dummy window created\n";
			#endif
		}

		~TrayIcon() {
			DestroyWindow(hWnd);
		}

		const void craeteTray() {
			NOTIFYICONDATA nid;
			ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
			nid = CREATE_NID_TEMPLATE;
			lstrcpy(nid.szTip, L"VanguardManager");
			lstrcpy(nid.szInfoTitle, L"Riot Vanguard Manager");
			lstrcpy(nid.szInfo, L"Vanguard manager up and running");
			BOOL result;
			result = Shell_NotifyIcon(NIM_ADD, &nid);
			if (!result) throw WinAPIException();

			#ifndef NDEBUG
			std::cout << "Tray icon created\n";
			#endif
		}

		void sendToastMessage(std::wstring message) {
			NOTIFYICONDATA nid = {
				.cbSize = sizeof(nid),
				.hWnd = hWnd,
				.uID = 1,
				.uFlags = NIF_INFO,
				.hIcon = LoadIcon(NULL, IDI_APPLICATION),
				.uTimeout = 5000,
				.dwInfoFlags = NIIF_USER,
			};
			nid = CREATE_NID_TEMPLATE;
			lstrcpy(nid.szTip, L"VanguardManager");
			lstrcpy(nid.szInfoTitle, L"Riot Vanguard Manager");
			lstrcpy(nid.szInfo, message.c_str());

			BOOL result;
			result = Shell_NotifyIcon(NIM_MODIFY, &nid);
			if (!result) throw WinAPIException();

			#ifndef NDEBUG
			std::cout << "Toast message sent\n";
			#endif
		}

	private:
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}
	};
};