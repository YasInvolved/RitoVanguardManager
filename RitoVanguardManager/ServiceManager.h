#pragma once

#include <Windows.h>
#include <stdexcept>

class ServiceManager {
private:
	SC_HANDLE schSCManager = NULL;
public:
	ServiceManager() {
		schSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
		if (schSCManager == NULL) {
			throw std::runtime_error("Failed to connect to windows service manager");
		}
	}

	bool runService(std::wstring serviceName) {
		SERVICE_STATUS_PROCESS ssStatus;
		DWORD dwBytesNeeded;
		DWORD dwStartTickCount;
		DWORD dwOldCheckPoint;
		DWORD dwWaitTime;

		SC_HANDLE schService = OpenService(
			schSCManager,
			serviceName.c_str(),
			SERVICE_ALL_ACCESS
		);

		if (schService == NULL) {
			CloseServiceHandle(schSCManager);
			throw std::runtime_error("Failed to start service");
		}

		// check service in case it's not stopped
		ssStatus = getServiceStatus(schService);

		// check if the service is already runnin
		if (ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING) {
			cleanupHandles(schService);
			throw std::runtime_error("Service already started");
		}

		// save the tick count
		dwStartTickCount = GetTickCount();
		dwOldCheckPoint = ssStatus.dwCheckPoint;

		while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING) {
			dwWaitTime = ssStatus.dwWaitHint / 10;

			if (dwWaitTime < 1000) dwWaitTime = 1000;
			else if (dwWaitTime > 10000) dwWaitTime = 10000;

			Sleep(dwWaitTime);

			// check if status is no longer pending
			ssStatus = getServiceStatus(schService);

			if (ssStatus.dwCheckPoint > dwOldCheckPoint) {
				dwStartTickCount = GetTickCount();
				dwOldCheckPoint = ssStatus.dwCheckPoint;
			}
			else {
				if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint) {
					cleanupHandles(schService);
					throw std::runtime_error("timeout waiting for service to stop");
				}
			}
		}

		if (!StartService(schService, 0, NULL)) {
			cleanupHandles(schService);
			throw std::runtime_error("Failed to start the service.");
		}

		ssStatus = getServiceStatus(schService);

		dwStartTickCount = GetTickCount();
		dwOldCheckPoint = ssStatus.dwCheckPoint;

		while (ssStatus.dwCurrentState == SERVICE_START_PENDING) {
			dwWaitTime = ssStatus.dwWaitHint / 10;

			if (dwWaitTime < 1000) dwWaitTime = 1000;
			else if (dwWaitTime > 10000) dwWaitTime = 10000;

			Sleep(dwWaitTime);

			ssStatus = getServiceStatus(schService);
			if (ssStatus.dwCheckPoint > dwOldCheckPoint) {
				dwStartTickCount = GetTickCount();
				dwOldCheckPoint = ssStatus.dwCheckPoint;
			}
			else {
				if (GetTickCount() - dwStartTickCount > ssStatus.dwWaitHint) break;
			}
		}

		if (ssStatus.dwCurrentState == SERVICE_RUNNING) MessageBox(NULL, L"Service running!", L"Success", MB_OK || MB_ICONINFORMATION);
		else {
			cleanupHandles(schService);
			throw std::runtime_error("Service not running!");
		}
	}

private:
	inline void cleanupHandles(SC_HANDLE serviceHandle) {
		CloseServiceHandle(serviceHandle);
		CloseServiceHandle(schSCManager);
	}

	inline SERVICE_STATUS_PROCESS getServiceStatus(SC_HANDLE serviceHandle) {
		SERVICE_STATUS_PROCESS ssStatus;
		DWORD dwBytesNeeded;
		if (!QueryServiceStatusEx(
			serviceHandle,
			SC_STATUS_PROCESS_INFO,
			(LPBYTE)&ssStatus,
			sizeof(SERVICE_STATUS_PROCESS),
			&dwBytesNeeded
		)) {
			cleanupHandles(serviceHandle);
			throw std::runtime_error("Failed to check service status");
		}
		return ssStatus;
	}
};
