#include "Settings.hpp"

#include <stdio.h>
#include <Windows.h>
#include "imgui.h"

#include "file.hpp"
#include "Common.hpp"
#include "Logs.hpp"
#include "TimeInfo.hpp"

const std::filesystem::path Settings::Default_Path = "settings.mto";
constexpr auto Reg_Path_Autorun = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr auto Reg_Value = L"Mes Touches";

std::optional<Settings> Settings::load_from_file(const std::filesystem::path& path) noexcept {
	auto opt_raw = file_read_byte(path);
	if (!opt_raw) return std::nullopt;

	auto& raw = *opt_raw;

	Settings set;

	size_t it = 0;
	if (raw.size() == 0) return std::nullopt;
	set.version = (uint8_t)raw[it];
	it++;

	if (raw.size() < it + 1) return std::nullopt;
	set.start_on_startup = (bool)raw[it];
	it++;

	return set;
}

bool Settings::save_to_file(const std::filesystem::path& path) noexcept {
	std::vector<std::byte> raw(2);
	raw[0] = (std::byte)version;
	raw[1] = (std::byte)start_on_startup;

	return file_write_byte(raw, path) == 0;
}

void Settings::copy_system() noexcept {
	HKEY hkey = NULL;
	start_on_startup = false;
	if (RegOpenKeyW(HKEY_CURRENT_USER, Reg_Path_Autorun, &hkey) == ERROR_SUCCESS) {
		DWORD type;
		start_on_startup =
			RegQueryValueExW(hkey, Reg_Value, NULL, &type, NULL, NULL) == ERROR_SUCCESS;
		RegCloseKey(hkey);
	}
}


[[nodiscard]] bool set_start_on_startup(bool v) noexcept {
	HKEY hkey = NULL;
	auto my_path = std::filesystem::current_path();

	auto format_message = [](auto m) {
		LPSTR buffer = nullptr;
		size_t size = FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			m,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&buffer,
			0,
			NULL
		);

		std::string message(buffer, size);
		LocalFree(buffer);
		return message;
	};

	if (v) {
		auto str = (my_path / "Mes_Touches.exe").native();

		auto result = RegCreateKeyW(HKEY_CURRENT_USER, Reg_Path_Autorun, &hkey);
	
		if (result != ERROR_SUCCESS) {
			ErrorDescription error;
			error.location = "set_start_on_startup";
			error.quick_desc = "Couldn't open the registry key to the start_on_startup flag.";
			error.message = format_message(result);
			error.type = ErrorDescription::Type::Reg;
			logs.lock_and_write(error);

			return false;
		}
		defer{ RegCloseKey(hkey); };

		result = RegSetValueExW(
			hkey,
			Reg_Value,
			0,
			REG_SZ,
			(BYTE*)str.c_str(),
			sizeof(std::remove_pointer_t<decltype(str.data())>) * (str.size() + 1)
		);

		if (result != ERROR_SUCCESS) {
			ErrorDescription error;
			error.location = "set_start_on_startup";
			error.quick_desc =
				"Couldn't set value of the registry key to the start_on_startup flag.";
			error.message = format_message(result);
			error.type = ErrorDescription::Type::Reg;
			logs.lock_and_write(error);

			return false;
		}

	}
	else {
		auto result = RegOpenKeyW(HKEY_CURRENT_USER, Reg_Path_Autorun, &hkey);

		if (result != ERROR_SUCCESS) {
			ErrorDescription error;
			error.location = "set_start_on_startup";
			error.quick_desc = "Couldn't open the registry key to the start_on_startup flag.";
			error.message = format_message(result);
			error.type = ErrorDescription::Type::Reg;
			logs.lock_and_write(error);

			return false;
		}

		defer { RegCloseKey(hkey); };

		result = RegDeleteValueW(hkey, Reg_Value);
		if (result != ERROR_SUCCESS) {
			ErrorDescription error;
			error.location = "set_start_on_startup";
			error.quick_desc = "Couldn't delete the registry key to the start_on_startup flag.";
			error.message = format_message(result);
			error.type = ErrorDescription::Type::Reg;
			logs.lock_and_write(error);
			return false;
		}
	}

	return true;
}

void SettingsWindow::render(Settings& settings) noexcept {
	constexpr time_t Reset_Down_Reset_Time{ 5 };

	ImGui::Begin("System");
	defer{ ImGui::End(); };

	if (ImGui::Checkbox("Start on startup", &settings.start_on_startup)) {
		settings.start_on_startup = set_start_on_startup(settings.start_on_startup);
	}
	
	if (reset_down_time_start > 0) {
		auto dt = (get_seconds_epoch() - reset_down_time_start);

		if (ImGui::Button("Sure ?")) {
			reset_keyboard_state = true;
			reset_mouse_state = true;
			reset_down_time_start = 0;
		}
		ImGui::SameLine();
		ImGui::Text("%zu s", Reset_Down_Reset_Time - dt + 1);

		if (dt > Reset_Down_Reset_Time) {
			reset_down_time_start = 0;
		}
	}
	else {
		if (ImGui::Button("Reset")) {
			reset_down_time_start = time(nullptr);
		}
	}

	if (ImGui::Button("Logs")) {
		show_log = true;
	}

	if (ImGui::Button("Quit")) {
		quit = true;
	}

	if (ImGui::Button("Install")) {
		install = true;
	}

	ImGui::SameLine();
	ImGui::Text("This will quit this window AND the hook process.");
}

