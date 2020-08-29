#pragma once

#include <vector>
#include <array>
#include <filesystem>
#include <optional>
#include <d3d9.h>

struct KeyEntry {
	static constexpr size_t Packed_Size = 9;

	uint8_t key_code;
	uint64_t timestamp;
};

extern const std::filesystem::path Default_Keyboard_Path;
constexpr size_t Keyboard_Save_Every_Mod{ 50 };

constexpr std::uint32_t Keyboard_File_Signature = 'BYEK'; // 'KEYB' byte swapped.

struct KeyboardState {
	uint8_t version_number;
	std::array<size_t, 0xff> key_times;
	std::vector<KeyEntry> key_entries;

	size_t modifications_since_save{ 0 };

	static std::optional<KeyboardState> load_from_file(std::filesystem::path path) noexcept;

	[[nodiscard]] bool save_to_file(std::filesystem::path path) const noexcept;
	
	std::array<size_t, 0xff> get_n_of_all_keys() const noexcept;
	void increment_key(KeyEntry key_entry) noexcept;

	void repair() noexcept;

	[[nodiscard]] bool reset_everything() noexcept;
};

struct KeyboardWindow {
	const size_t Reset_Button_Time{ 5 };

	bool reset{ false };
	bool save{ false };
	bool reload{ false };

	bool render_key_list_checkbox = false;
	size_t reset_button_timer = Reset_Button_Time;
	time_t reset_time_start = 0;

	void render(std::optional<KeyboardState>& state) noexcept;
};
