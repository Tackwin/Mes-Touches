#if 1

#include <Windows.h>
#include <stdio.h>
#include <array>
#include "psapi.h"

#ifdef ARCH_32
#define install_hook   install_hook_32
#define uninstall_hook uninstall_hook_32
#endif
#ifdef ARCH_64
#define install_hook   install_hook_64
#define uninstall_hook uninstall_hook_64
#endif


constexpr auto Mail_Name = "\\\\.\\Mailslot\\Mes Touches";

void open_mail(const char* mail_name) noexcept;
void write_mail(int n_code, WPARAM w_param, LPARAM l_param) noexcept;

HHOOK hook_handle = nullptr;
HMODULE handle_module = nullptr;
HANDLE mail_slot = nullptr;
UINT msg = 0;

LRESULT CALLBACK hook(int n_code, WPARAM w_param, LPARAM l_param) noexcept {
	if (!mail_slot) open_mail(Mail_Name);
	if (!msg) msg = RegisterWindowMessage(Mail_Name);

	if (n_code && mail_slot) {
		switch(n_code) {
		case HCBT_CREATEWND:
		case HCBT_DESTROYWND: {

			write_mail(n_code, w_param, l_param);
			PostMessage(HWND_BROADCAST, msg, 0, 0);
		}}
	}
	return CallNextHookEx(NULL, n_code, w_param, l_param);
}

__declspec(dllexport)
bool install_hook() {
	hook_handle = SetWindowsHookEx(WH_CBT, hook, handle_module, NULL);
	return hook_handle != nullptr;
}

__declspec(dllexport)
bool uninstall_hook() {
	if (!hook_handle) return false;
	auto res = UnhookWindowsHookEx(hook_handle) != NULL;
	if (res) {
		hook_handle = nullptr;
		DWORD_PTR dwResult;
		// wakeup everyone so they can free themselves.
		SendMessageTimeout(
			HWND_BROADCAST,
			WM_NULL,
			0,
			0,
			SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG,
			1000,
			&dwResult
		);
	}
	return res;
}

#if 0
#include <utility>

CALLBACK BOOL helper(HWND hwnd, LPARAM lParam) {
	auto pParams = (std::pair<HWND, DWORD>*)(lParam);

	DWORD processId;
	if (GetWindowThreadProcessId(hwnd, &processId) && processId == pParams->second)
	{
		// Stop enumerating
		SetLastError(-1);
		pParams->first = hwnd;
		return FALSE;
	}

	// Continue enumerating
	return TRUE;
}

HWND FindTopWindow(DWORD pid)
{
    std::pair<HWND, DWORD> params = { 0, pid };

    // Enumerate the windows using a lambda to process each window
    BOOL bResult = EnumWindows(helper, (LPARAM)&params);

    if (!bResult && GetLastError() == -1 && params.first)
    {
        return params.first;
    }

    return 0;
}
#endif

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
	handle_module = module;

#if 0
	WCHAR wide_buffer[MAX_PATH] = {};
	DWORD proc_id = GetCurrentProcessId();
	auto handle_process =
		OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, proc_id);

	GetModuleFileNameExW(handle_process, NULL, wide_buffer, MAX_PATH);
	OutputDebugStringW(L"Started A:");
	OutputDebugStringW(wide_buffer);
	if (FindTopWindow(proc_id)) GetWindowTextW(FindTopWindow(proc_id), wide_buffer, MAX_PATH);
	OutputDebugStringW(wide_buffer);
#endif

	return true;
}

void open_mail(const char* mail_name) noexcept {
	mail_slot = CreateFile(
		mail_name,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);
}

void write_mail(int n_code, WPARAM w_param, LPARAM l_param) noexcept {
	DWORD cbWritten;

	struct Msg {
		int code;
		WPARAM w_param;
		LPARAM l_param;
	} msg = { n_code, w_param, l_param };

	WriteFile(
		mail_slot,
		reinterpret_cast<const char*>(&msg),
		sizeof(msg),
		&cbWritten,
		nullptr
	);
}
#else
#include <Windows.h>
#include <stdio.h>
#include <array>
#include "psapi.h"

constexpr char* Mail_Name = "\\\\.\\Mailslot\\Mes Touches";


__declspec(dllexport)
bool uninstall_hook();
void open_mail(const char* mail_name) noexcept;
void write_mail(int n_code, WPARAM w_param, LPARAM l_param) noexcept;

HHOOK hook_handle = nullptr;
HMODULE handle_module = nullptr;
HANDLE mail_slot = nullptr;
UINT msg = 0;

LRESULT CALLBACK hook(int n_code, WPARAM w_param, LPARAM l_param) noexcept {
	if (!mail_slot) open_mail(Mail_Name);
	if (!msg) msg = RegisterWindowMessage(Mail_Name);

	if (n_code && mail_slot) {
		switch(n_code) {
		case HCBT_CREATEWND:
		case HCBT_DESTROYWND: {
			if ((HWND)w_param != GetAncestor((HWND)w_param, GA_ROOT))
				break;

			write_mail(n_code, w_param, l_param);
			PostMessage(HWND_BROADCAST, msg, 0, 0);
		}}
	}
	return CallNextHookEx(NULL, n_code, w_param, l_param);
}

__declspec(dllexport)
bool install_hook() {
	hook_handle = SetWindowsHookEx(WH_CBT, hook, handle_module, NULL);
	return hook_handle != nullptr;
}

__declspec(dllexport)
bool uninstall_hook() {
	if (!hook_handle) return false;
	auto res = UnhookWindowsHookEx(hook_handle) != NULL;
	if (res) {
		hook_handle = nullptr;
		DWORD_PTR dwResult;
		// wakeup everyone so they can free themselves.
		SendMessageTimeout(
			HWND_BROADCAST,
			WM_NULL,
			0,
			0,
			SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG,
			1000,
			&dwResult
		);
	}
	return res;
}
#include <utility>

HWND FindTopWindow(DWORD pid)
{
    std::pair<HWND, DWORD> params = { 0, pid };

    // Enumerate the windows using a lambda to process each window
    BOOL bResult = EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL 
    {
        auto pParams = (std::pair<HWND, DWORD>*)(lParam);

        DWORD processId;
        if (GetWindowThreadProcessId(hwnd, &processId) && processId == pParams->second)
        {
            // Stop enumerating
            SetLastError(-1);
            pParams->first = hwnd;
            return FALSE;
        }

        // Continue enumerating
        return TRUE;
    }, (LPARAM)&params);

    if (!bResult && GetLastError() == -1 && params.first)
    {
        return params.first;
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
	handle_module = module;


	WCHAR wide_buffer[MAX_PATH] = {};
	DWORD proc_id = GetCurrentProcessId();
	auto handle_process =
		OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, proc_id);

	GetModuleFileNameExW(handle_process, NULL, wide_buffer, MAX_PATH);
	if (FindTopWindow(proc_id)) GetWindowTextW(FindTopWindow(proc_id), wide_buffer, MAX_PATH);


	return true;
}

void open_mail(const char* mail_name) noexcept {
	mail_slot = CreateFile(
		mail_name,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);
}

void write_mail(int n_code, WPARAM w_param, LPARAM l_param) noexcept {
	DWORD cbWritten;

	struct Msg {
		int code;
		WPARAM w_param;
		LPARAM l_param;
	} msg = { n_code, w_param, l_param };

	WriteFile(
		mail_slot,
		reinterpret_cast<const char*>(&msg),
		sizeof(msg),
		&cbWritten,
		nullptr
	);
}
#endif
