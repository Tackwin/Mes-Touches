#include "NotifyIcon.hpp"
#include <cassert>

#include "Common.hpp"
#include "Logs.hpp"
#include "ErrorCode.hpp"

NotifyIcon::NotifyIcon() noexcept {
	data.cbSize = sizeof data;
	data.uFlags = NIF_STATE | NIF_GUID | NIF_SHOWTIP;
	data.dwState = NIS_HIDDEN;
	data.dwStateMask = data.dwState;
	data.uVersion = NOTIFYICON_VERSION_4;
	data.dwInfoFlags = NIIF_USER;
}

void NotifyIcon::update() noexcept {
	if (!icon_added) return;

	auto result = Shell_NotifyIcon(NIM_MODIFY, &data);
	
	if (!result) {
		ErrorDescription error;
		error.location = "NotifyIcon::update";
		error.quick_desc = "Shell_NotifyIcon(NIM_MODIFY, &data) => Unknown error.";
		error.message = format_error_code(GetLastError());
		error.type = ErrorDescription::Type::Notify;
		logs.lock_and_write(error);
	}
}

void NotifyIcon::setup_notify_icon(
	HWND window, HICON ico, const std::string& tip, size_t msg
) noexcept {
	set_window(window);
	set_icon(ico);
	set_tooltip(tip);
	set_message(msg);
	add();
	show();
}

void NotifyIcon::set_window(HWND window) noexcept {
	data.hWnd = window;
	update();
}

void NotifyIcon::set_icon(HICON ico) noexcept {
	data.hIcon = ico;
	data.uFlags |= NIF_ICON;
	update();
}

void NotifyIcon::set_tooltip(const std::string& tip) noexcept {
	constexpr auto MAX_LENGTH_TIP = 128;
	assert(tip.size() < MAX_LENGTH_TIP);
	memcpy(data.szTip, tip.c_str(), tip.size() + 1);
	data.uFlags |= NIF_TIP;
	update();
}

void NotifyIcon::set_message(size_t msg) noexcept {
	data.uCallbackMessage = msg;
	data.uFlags |= NIF_MESSAGE;
	update();
}

void NotifyIcon::remove() noexcept {
	BOOL ret = Shell_NotifyIcon(NIM_DELETE, &data);
	if (!ret) {
		ErrorDescription error;
		error.location = "NotifyIcon::remove";
		error.quick_desc = "Shell_NotifyIcon(NIM_DELETE, &data) => Unknown error.";
		error.message = format_error_code(GetLastError());
		error.type = ErrorDescription::Type::Notify;
		logs.lock_and_write(error);
		return;
	}
	icon_added = false;
}


void NotifyIcon::add() noexcept {
	BOOL ret = Shell_NotifyIcon(NIM_ADD, &data);
	if (!ret) {
		ErrorDescription error;
		error.location = "NotifyIcon::add";
		error.quick_desc = "Shell_NotifyIcon(NIM_ADD, &data) => Unknown error.";
		error.message = format_error_code(GetLastError());
		error.type = ErrorDescription::Type::Notify;
		logs.lock_and_write(error);
		return;
	}
	ret = Shell_NotifyIcon(NIM_SETVERSION, &data);
	if (!ret) {
		ErrorDescription error;
		error.location = "NotifyIcon::add";
		error.quick_desc = "Shell_NotifyIcon(NIM_SETVERSION, &data) => Unknown error.";
		error.message = format_error_code(GetLastError());
		error.type = ErrorDescription::Type::Notify;
		logs.lock_and_write(error);
		return;
	}
	icon_added = true;
}

void NotifyIcon::show() noexcept {
	data.dwState &= ~NIS_HIDDEN;
	update();
}
