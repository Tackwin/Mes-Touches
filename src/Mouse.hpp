#pragma once
#include <filesystem>
#include <optional>
#include <vector>
#include <array>
#include <algorithm>
#include <unordered_map>
#include <string_view>

struct ClickEntry {
	static constexpr size_t Byte_Size = 17;

	std::uint8_t specify_display;
	std::uint8_t wheel_scrolled;
	std::uint8_t wheel_delta;
	std::uint8_t button_code;
	std::uint32_t x;
	std::uint32_t y;
	std::uint64_t timestamp;
};

struct Display {
	static constexpr size_t Unique_Hash_Size = 32;
	static constexpr size_t Custom_Name_Size = 32;
	static constexpr size_t Byte_Size = 32 + Unique_Hash_Size + Custom_Name_Size;

	std::uint32_t width;
	std::uint32_t height;
	std::uint32_t x;
	std::uint32_t y;
	char unique_hash_char[Unique_Hash_Size];
	char custom_name[Unique_Hash_Size];
	std::uint64_t timestamp_start;
	std::uint64_t timestamp_end{ 0 };
};

constexpr std::uint32_t Mouse_File_Signature = 'SUOM'; // 'MOUS' byte swapped.

struct MouseStateCache {
	template<typename T>
	struct Cached {
		bool dirty = true;
		T v;
	};
	template<typename T>
	struct Cached_Herited : public T {
		bool dirty = true;
	};

	struct UsagePlot {
		size_t rolling_average = 1; // seconds;
		size_t resolution = 100;
		std::vector<float> values;
	};

	std::unordered_map<std::string_view, Cached<size_t>> n_keys;
	Cached_Herited<UsagePlot> usage_plot;
};

struct MouseState {
	static const std::filesystem::path Default_Path;
	static const size_t Save_Every_Mod;
	
	enum class ButtonMap : uint8_t {
		Left = 0,
		Wheel,
		Right,
		Mouse_3,
		Mouse_4,
		Mouse_5,
		Wheel_Up,
		Wheel_Down,
		Count
	};
	static constexpr size_t N_Button_Supported = 32 > (int)ButtonMap::Count ? 32 : (int)ButtonMap::Count;

	uint8_t version_number;
	std::vector<ClickEntry> click_entries;
	std::vector<Display> display_entries;
	std::array<size_t, N_Button_Supported + 2> buttons;

	size_t modifications_since_save{ 0 };

	[[nodiscard]]
	static std::optional<MouseState> load_from_file(
		const std::filesystem::path& path, bool strict
	) noexcept;
	[[nodiscard]] bool save_to_file(const std::filesystem::path& path) noexcept;

	size_t increment_button(ClickEntry click) noexcept;

	[[nodiscard]] bool reset_everything() noexcept;
	[[nodiscard]] bool save_blank_state(const std::filesystem::path& path) noexcept;

	void remove_display(size_t display_idx) noexcept;

	mutable MouseStateCache cache;
};

struct MouseWindow {
	const size_t Reset_Button_Time = 5;

	bool render_buttons_list_checkbox = false;
	size_t reset_button_timer = Reset_Button_Time;
	time_t reset_time_start = 0;

	size_t display_list_selected = 0;
	bool display_list_focused = false;

	bool reset{ false };
	bool strict{ true };
	bool reload{ false };
	bool save{ false };

	void render(std::optional<MouseState>& state) noexcept;
};
