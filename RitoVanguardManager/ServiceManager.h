#pragma once
#include <stdexcept>
#include <Windows.h>

class ServiceManager {
public:
	ServiceManager() {
		SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (schSCManager == NULL) {
			throw std::runtime_error("Failed to start service manager");
		}
	}
};