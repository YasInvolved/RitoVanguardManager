#include <string>
#include <sstream>
#include <stdexcept>
#include <Windows.h>
#include <memory>
#include <vector>
#include <psapi.h>
#include <filesystem>
#include <fstream>

#include "ServiceManager.h"
#include "tray.h"
#include "WinAPIException.h"

// just to not forget: ".../RiotClientServices.exe" --launch-product=league_of_legends --launch-patchline=live

constexpr wchar_t VANGUARD_SERVICE_NAME[] = L"vgc";
constexpr wchar_t CLIENT_ARGS[] = L"--launch-product=league_of_legends --launch-patchline=live";

namespace VanguardManager {
	class App {
	private:
		std::unique_ptr<ServiceManager> serviceManager; // service manager requires to be started later cuz it requires administrator permissions
		std::unique_ptr<TrayIcon> trayIcon = std::make_unique<TrayIcon>();
		std::wstring executablePath = L"";
	public:
		void run() {
			// check if application is ran as administrator
			elevate();
			serviceManager = std::make_unique<ServiceManager>();
			enableTrayIcon();
			getStartPath();
			startVanguard();
			startRiotClient();
			listenToLeagueProcess();
			stopVanguard();
		}

	private:
		inline void startVanguard() {
			serviceManager->runService(VANGUARD_SERVICE_NAME);
		}

		inline void enableTrayIcon() {
			trayIcon->craeteTray();
		}

		void getStartPath() {
			std::wifstream config(L"VanguardManager.conf");
			if (!config.good()) throw std::runtime_error("Couldn't open config file");
			std::getline(config, executablePath);
		}

		void startRiotClient() {
			std::wstringstream cmd;
			cmd << L'\"' << executablePath << L'\"' << CLIENT_ARGS;
			PROCESS_INFORMATION pi;
			STARTUPINFO si = { sizeof(si) };
			BOOL result = CreateProcessW(
				NULL, const_cast<wchar_t*>(cmd.str().c_str()),
				NULL, NULL, FALSE,
				0, NULL, NULL, &si, &pi
			);

			if (!result) throw WinAPIException();

			#ifndef NDEBUG
			std::wcout << L"Riot Client is running\nPID = " << pi.dwProcessId << "\n ";
			#endif

			DWORD status = WaitForSingleObject(pi.hProcess, INFINITE);

			#ifndef NDEBUG
			if (status == WAIT_OBJECT_0) {
				std::wcout << L"Riot Client has stopped\n";
			}
			#endif

			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}

		void listenToLeagueProcess() {
			DWORD Procs[1024], bytesReturned, NumOfProcesses;
			DWORD leagueProcessPID = 0;
			TCHAR szProcessName[MAX_PATH];
			
			bool leagueProcessFound = false;
			do {
				if (!EnumProcesses(Procs, sizeof(Procs), &bytesReturned)) throw WinAPIException();
				NumOfProcesses = bytesReturned / sizeof(DWORD);
				for (int i = 0; i < NumOfProcesses; i++) {
					if (Procs[i] != 0) {
						HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, Procs[i]);
						if (hProc != NULL) {
							HMODULE hMod = NULL;
							DWORD modBytesNeeded = 0;
							EnumProcessModulesEx(hProc, &hMod, sizeof(HMODULE), &modBytesNeeded, LIST_MODULES_64BIT);
							if (hMod != NULL) {
								GetModuleBaseName(hProc, hMod, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));
								if (lstrcmp((wchar_t*)szProcessName, L"LeagueClientUx.exe") == 0) {
									#ifndef NDEBUG
									std::wcout << L"FOUND PID: " << Procs[i] << L" Name: " << (wchar_t*)szProcessName << L'\n';
									#endif
									leagueProcessPID = Procs[i];
									leagueProcessFound = true;
									break;
								}
								CloseHandle(hProc);
							}
						}
					}
				}
			} while (!leagueProcessFound);
			
			std::wstringstream msg;
			msg << L"Connected to League of Legends (PID: " << leagueProcessPID << L')';
			trayIcon->sendToastMessage(msg.str());

			HANDLE hLProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, leagueProcessPID);
			DWORD status = WaitForSingleObject(hLProc, INFINITE); // wait for league process to exit
			#ifndef NDEBUG
			if (status == WAIT_OBJECT_0) {
				std::wcout << L"League has stopped\n";
			}
			#endif
			CloseHandle(hLProc);
		}

		inline void stopVanguard() {
			serviceManager->stopService(VANGUARD_SERVICE_NAME);
		}

		BOOL isElevated() {
			BOOL IsElevated = FALSE;
			DWORD dwError = ERROR_SUCCESS;
			PSID pAdministratorsGroup = NULL;

			SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
			if (!AllocateAndInitializeSid(
				&NtAuthority,
				2,
				SECURITY_BUILTIN_DOMAIN_RID,
				DOMAIN_ALIAS_RID_ADMINS,
				0, 0, 0, 0, 0, 0,
				&pAdministratorsGroup
			)) {
				dwError = GetLastError();
				goto Cleanup;
			}

			if (!CheckTokenMembership(NULL, pAdministratorsGroup, &IsElevated)) {
				dwError = GetLastError();
				goto Cleanup;
			}

		Cleanup:
			if (pAdministratorsGroup)
			{
				FreeSid(pAdministratorsGroup);
				pAdministratorsGroup = NULL;
			}

			if (ERROR_SUCCESS != dwError) {
				throw dwError;
			}

			return IsElevated;
		}

		void elevate() {
			// no need to elevate if elevated
			if (isElevated()) return;

			wchar_t path[MAX_PATH];
			if (GetModuleFileName(NULL, path, ARRAYSIZE(path))) {
				SHELLEXECUTEINFO sei = { sizeof(sei) };
				sei.lpVerb = L"runas";
				sei.lpFile = path;
				sei.hwnd = NULL;
				sei.nShow = SW_NORMAL;

				if (!ShellExecuteEx(&sei)) {
					DWORD dwError = GetLastError();
					if (dwError == ERROR_CANCELLED) throw std::runtime_error("Accept this next time");
					throw std::runtime_error("Unknown error occured");
				}
			}
			exit(0);
		}

		static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}
	};
};

#ifdef NDEBUG
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdline, int nCmdShow) {
	VanguardManager::App* app = new VanguardManager::App();
	try {
		app->run();
	}
	catch (std::runtime_error& e) {
		MessageBoxA(0, e.what(), "error occured", MB_OK);
		exit(-1);
	}
	catch (WinAPIException& e) {
		MessageBoxA(0, e.what().c_str(), "error occured", MB_OK);
		exit(-2);
	}
}

#else

int main(int argc, char** argv) {
	VanguardManager::App* app = new VanguardManager::App();
	try {
		app->run();
	}
	catch (const std::runtime_error& e) {
		std::cout << "Error: " << e.what() << "\n";
		system("pause");
		exit(-1);
	}
	catch (WinAPIException& e) {
		std::cout << "WinAPI Error: " << e.what() << "\n";
		system("pause");
		exit(-2);
	}
}

#endif