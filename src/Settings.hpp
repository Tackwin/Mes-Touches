#pragma once
#include <optional>
#include <filesystem>

struct Settings {
	static const std::filesystem::path Default_Path;

	uint8_t version{ 0 };
	bool start_on_startup{ false };
	bool show_logs{ false };

	static std::optional<Settings> load_from_file(const std::filesystem::path& path) noexcept;

	void copy_system() noexcept;
	bool save_to_file(const std::filesystem::path& path) noexcept;
};

struct SettingsWindow {
	bool reset_keyboard_state{ false };
	bool reset_mouse_state{ false };
	bool quit{ false };
	bool show_log{ false };

	bool install{ false };

	time_t reset_down_time_start{ 0 };

	void render(Settings& settings) noexcept;
};

[[nodiscard]] extern bool set_start_on_startup(bool v) noexcept;

