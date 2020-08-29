#pragma once
#include <Windows.h>
#include <string>
#include <shellapi.h>

struct NotifyIcon {
	NotifyIcon() noexcept;

	void setup_notify_icon(HWND window, HICON ico, const std::string& tip, size_t msg) noexcept;

	void set_window(HWND window) noexcept;
	void set_icon(HICON ico) noexcept;
	void set_tooltip(const std::string& tip) noexcept;
	void set_message(size_t msg) noexcept;
	void remove() noexcept;
	void show() noexcept;
	void add() noexcept;

	NOTIFYICONDATA data{};
	bool icon_added{ false };

private:
	void update() noexcept;

	HMENU menu = nullptr;
};
