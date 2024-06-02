#pragma once

#include <Windows.h>
#include <stdexcept>
#include <memory>

#include "NonCopyable.h"
#include "WinAPIException.h"

#ifndef NDEBUG
#include <iostream>
#endif

namespace VanguardManager {
	class ServiceManager : NonCopyable {
	private:
		SC_HANDLE schSCManager = NULL;
	public:
		ServiceManager() {
			schSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
			if (schSCManager == NULL) {
				throw std::runtime_error("Failed to connect to windows service manager");
			}
		}

		~ServiceManager() {
			CloseServiceHandle(schSCManager);
		}

		void runService(std::wstring serviceName) {
			SERVICE_STATUS_PROCESS ssStatus;
			DWORD dwBytesNeeded = 0;
			ULONGLONG dwStartTickCount;
			DWORD dwOldCheckPoint;
			DWORD dwWaitTime;

			SC_HANDLE schService = OpenService(
				schSCManager,
				serviceName.c_str(),
				SERVICE_ALL_ACCESS
			);

			if (schService == NULL) {
				throw WinAPIException();
			}

			// check service in case it's not stopped
			ssStatus = getServiceStatus(schService);

			// check if the service is already runnin
			if (ssStatus.dwCurrentState != SERVICE_STOPPED && ssStatus.dwCurrentState != SERVICE_STOP_PENDING) {
				CloseServiceHandle(schService);

				#ifndef NDEBUG
				std::wcout << L"Service " << serviceName << L" is already running\n";
				#endif

				return;
			}

			// save the tick count
			dwStartTickCount = GetTickCount64();
			dwOldCheckPoint = ssStatus.dwCheckPoint;

			while (ssStatus.dwCurrentState == SERVICE_STOP_PENDING) {
				dwWaitTime = ssStatus.dwWaitHint / 10;

				if (dwWaitTime < 1000) dwWaitTime = 1000;
				else if (dwWaitTime > 10000) dwWaitTime = 10000;

				Sleep(dwWaitTime);

				// check if status is no longer pending
				ssStatus = getServiceStatus(schService);

				if (ssStatus.dwCheckPoint > dwOldCheckPoint) {
					dwStartTickCount = GetTickCount64();
					dwOldCheckPoint = ssStatus.dwCheckPoint;
				}
				else {
					if (GetTickCount64() - dwStartTickCount > ssStatus.dwWaitHint) {
						CloseServiceHandle(schService);
						throw std::runtime_error("timeout waiting for service to stop");
					}
				}
			}

			if (!StartService(schService, 0, NULL)) {
				CloseServiceHandle(schService);
				throw std::runtime_error("Failed to start the service.");
			}

			ssStatus = getServiceStatus(schService);

			dwStartTickCount = GetTickCount64();
			dwOldCheckPoint = ssStatus.dwCheckPoint;

			while (ssStatus.dwCurrentState == SERVICE_START_PENDING) {
				dwWaitTime = ssStatus.dwWaitHint / 10;

				if (dwWaitTime < 1000) dwWaitTime = 1000;
				else if (dwWaitTime > 10000) dwWaitTime = 10000;

				Sleep(dwWaitTime);

				ssStatus = getServiceStatus(schService);
				if (ssStatus.dwCheckPoint > dwOldCheckPoint) {
					dwStartTickCount = GetTickCount64();
					dwOldCheckPoint = ssStatus.dwCheckPoint;
				}
				else {
					if (GetTickCount64() - dwStartTickCount > ssStatus.dwWaitHint) break;
				}
			}
			#ifndef NDEBUG
			if (ssStatus.dwCurrentState == SERVICE_RUNNING)
				std::wcout << L"Service running!";
			else {
				CloseServiceHandle(schService);
				throw std::runtime_error("Service not running!\n");
			}
			#else
			if (ssStatus.dwCurrentState != SERVICE_RUNNING) {
				CloseServiceHandle(schService);
				throw std::runtime_error("Service not running!\n");
			}
			#endif
		}


		void stopService(std::wstring serviceName) {
			SERVICE_STATUS_PROCESS serviceStatus;
			DWORD dwStartTime = GetTickCount64();
			DWORD dwBytesNeeded;
			DWORD dwTimeout = 30000;
			DWORD dwWaitTime;

			SC_HANDLE serviceHandle = OpenService(
				schSCManager,
				serviceName.c_str(),
				SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS
			);

			if (serviceHandle == NULL) {
				throw std::runtime_error("Failed to access desired service.");
			}

			serviceStatus = getServiceStatus(serviceHandle);
			if (serviceStatus.dwCurrentState == SERVICE_STOPPED)
				#ifndef NDEBUG
				std::wcout << L"Service already stopped\n";
				#endif
			// wait for service stop if it's pending
			while (serviceStatus.dwCurrentState == SERVICE_STOP_PENDING) {
				dwWaitTime = serviceStatus.dwWaitHint / 10;

				if (dwWaitTime < 1000) dwWaitTime = 1000;
				else if (dwWaitTime > 10000) dwWaitTime = 10000;

				Sleep(dwWaitTime);

				serviceStatus = getServiceStatus(serviceHandle);

				if (serviceStatus.dwCurrentState == SERVICE_STOPPED) break;

				if (GetTickCount64() - dwStartTime > dwTimeout) {
					#ifndef NDEBUG
					std::wcout << L"Service stop timeout\n";
					#endif
					CloseServiceHandle(serviceHandle);
					return;
				}
			}
		}

	private:
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
				CloseServiceHandle(serviceHandle);
				throw WinAPIException();
			}
			return ssStatus;
		}
	};
};
