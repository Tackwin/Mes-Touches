#pragma once
#include <map>
#include <set>
#include <string>
#include <cstdint>
#include <optional>
#include <filesystem>
#include <array>

#include "xstd.hpp"

struct AppUsage {
	static constexpr size_t Id = 0;
	static constexpr size_t Max_String_Size = 128;
	static constexpr size_t Byte_Size = Max_String_Size * 2 + 16;

	using Stack_String = std::array<char, Max_String_Size>;

	Stack_String exe_name = {};
	Stack_String doc_name = {};
	std::uint64_t timestamp_start;
	std::uint64_t timestamp_end;
};

constexpr std::uint32_t Event_File_Signature = 'NEVE'; // 'EVEN' byte swapped.

struct EventCache {
	bool dirty = true;
	std::map<AppUsage::Stack_String, uint64_t> exe_to_time;
	std::vector<AppUsage::Stack_String> exe_to_time_sorted;
	std::map<AppUsage::Stack_String, uint64_t> doc_to_time;
	std::map<AppUsage::Stack_String, std::set<AppUsage::Stack_String>>
		exe_to_docs;
	std::map<AppUsage::Stack_String, std::vector<AppUsage::Stack_String>>
		exe_to_docs_sorted;
};

struct EventState {
	inline static const std::filesystem::path Default_Path = "event.mto";
	inline static const size_t Save_Every_Mod = 50;

	mutable EventCache cache;

	double last_update_countdown = 0.0;

	std::vector<AppUsage> apps_usages;

	size_t modifications_since_save{ 0 };

	[[nodiscard]] static std::optional<EventState> load_from_file(
		std::filesystem::path path
	) noexcept;
	[[nodiscard]] bool save_to_file(std::filesystem::path path) noexcept;


	void register_event(AppUsage event) noexcept;

	void check_resave() noexcept;

	bool reset_everything() noexcept;
};

struct EventWindow {
	static constexpr size_t Reset_Button_Time = 5;

	bool render_buttons_list_checkbox = false;
	size_t reset_button_timer = Reset_Button_Time;
	time_t reset_time_start = 0;

	bool sort_less{ false };

	bool save{ false };
	bool reset{ false };
	bool strict{ false };
	bool reload{ false };

	void render(std::optional<EventState>& state) noexcept;

};