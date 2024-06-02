#pragma once
#include <stdexcept>
#include <Windows.h>

class WinAPIException : std::exception {
public:
	const std::string what() {
		DWORD lastError = GetLastError();
		if (lastError == 0) throw std::runtime_error("Unknown exception occured");

		LPSTR messageBuffer = nullptr;
		size_t size = FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			lastError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&messageBuffer, 0, NULL
		);

		std::string message(messageBuffer, size);
		LocalFree(messageBuffer);

		return message;
	}
};