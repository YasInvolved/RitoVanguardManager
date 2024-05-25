#include <string>
#include <stdexcept>
#include <Windows.h>

#include "ServiceManager.h"

#define VANGUARD_SERVICE_NAME "vgc"

class App {
private:
public:
	void run() {
		// check if application is ran as administrator
		elevate();
		cleanup();
	}

	void cleanup() {

	}

private:
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

		wchar_t szPath[MAX_PATH];
		if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath))) {
			SHELLEXECUTEINFO sei = { sizeof(sei) };
			sei.lpVerb = L"runas";
			sei.lpFile = szPath;
			sei.hwnd = NULL;
			sei.nShow = SW_NORMAL;

			if (!ShellExecuteEx(&sei)) {
				DWORD dwError = GetLastError();
				if (dwError == ERROR_CANCELLED) throw std::runtime_error("Accept this next time");
				throw std::runtime_error("Unknown error occured");
			}
		}
	}
};

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdline, int nCmdShow) {
	App* app = new App();
	try {
		app->run();
	}
	catch (std::runtime_error& e) {
		MessageBoxA(0, e.what(), "error occured", MB_OK);
		app->cleanup();
		exit(-1);
	}
}