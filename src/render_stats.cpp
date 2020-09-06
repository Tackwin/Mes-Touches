#include "render_stats.hpp"
#include <Windows.h>
#include "imgui.h"
#include "imgui_ext.h"
#include "Common.hpp"
#include <string>
#include <algorithm>

std::string get_name_of_key(uint8_t key_code) noexcept {
	switch (key_code)
	{
	#define X(x) x: return #x
		case X(VK_NUMPAD0	);
		case X(VK_NUMPAD1	);
		case X(VK_NUMPAD2	);
		case X(VK_NUMPAD3	);
		case X(VK_NUMPAD4	);
		case X(VK_NUMPAD5	);
		case X(VK_NUMPAD6	);
		case X(VK_NUMPAD7	);
		case X(VK_NUMPAD8	);
		case X(VK_NUMPAD9	);
		case X(VK_SEPARATOR	);
		case X(VK_SUBTRACT	);
		case X(VK_DECIMAL	);
		case X(VK_DIVIDE	);
		case X(VK_F1		);
		case X(VK_F2		);
		case X(VK_F3		);
		case X(VK_F4		);
		case X(VK_F5		);
		case X(VK_F6		);
		case X(VK_F7		);
		case X(VK_F8		);
		case X(VK_F9		);
		case X(VK_F10		);
		case X(VK_F11		);
		case X(VK_F12		);
		case X(VK_F13		);
		case X(VK_F14		);
		case X(VK_F15		);
		case X(VK_F16		);
		case X(VK_F17		);
		case X(VK_F18		);
		case X(VK_F19		);
		case X(VK_F20		);
		case X(VK_F21		);
		case X(VK_F22		);
		case X(VK_F23		);
		case X(VK_F24		);
		case X(VK_NUMLOCK	);
		case X(VK_SCROLL	);
		case X(VK_LSHIFT	);
		case X(VK_RSHIFT	);
		case X(VK_LCONTROL	);
		case X(VK_RCONTROL	);
		case X(VK_LMENU		);
		case X(VK_RMENU		);
		case X(VK_PLAY		);
		case X(VK_ZOOM		);
		case X(VK_LBUTTON	);
		case X(VK_RBUTTON	);
		case X(VK_CANCEL	);
		case X(VK_MBUTTON	);
		case X(VK_BACK		);
		case X(VK_TAB		);
		case X(VK_CLEAR		);
		case X(VK_RETURN	);
		case X(VK_SHIFT		);
		case X(VK_CONTROL	);
		case X(VK_MENU		);
		case X(VK_PAUSE		);
		case X(VK_CAPITAL	);
		case X(VK_ESCAPE	);
		case X(VK_SPACE		);
		case X(VK_PRIOR		);
		case X(VK_NEXT		);
		case X(VK_END		);
		case X(VK_HOME		);
		case X(VK_LEFT		);
		case X(VK_UP		);
		case X(VK_RIGHT		);
		case X(VK_DOWN		);
		case X(VK_SELECT	);
		case X(VK_PRINT		);
		case X(VK_EXECUTE	);
		case X(VK_SNAPSHOT	);
		case X(VK_INSERT	);
		case X(VK_DELETE	);
		case X(VK_HELP		);
#undef X
		case 190: return ".";
		case 0x07: case 0x0A: case 0x0B: case 0x0E: case 0x0F: case 0x16: case 0x1A: case 0x5E:
		case 0x97: case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9E:
		case 0x9F: case 0xB8: case 0xB9: case 0xC1: case 0xC2: case 0xC3: case 0xC4: case 0xC5:
		case 0xC6: case 0xC7: case 0xC8: case 0xC9: case 0xCA: case 0xCB: case 0xCC: case 0xCD:
		case 0xCE: case 0xCF: case 0xD0: case 0xD1: case 0xD2: case 0xD3: case 0xD4: case 0xD5:
		case 0xD6: case 0xD7: case 0xD8: case 0xD9: case 0xDA: case 0xE0: case 0xE8:
		// reserved we do nothing.

		case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0xE1: case 0xE3: case 0xE4:
		case 0xE6: case 0xE9: case 0xEA: case 0xEB: case 0xEC: case 0xED: case 0xEE: case 0xEF:
		case 0xF1: case 0xF2: case 0xF3: case 0xF4: case 0xF5:
		//>TODO OEM specific
			return "OEM specific";
	default:
		// so now we are in the 0-Z ascii range so we can just cast to a char.

		return { (char)key_code };
		break;
	}
}
std::string get_name_of_mouse_button(uint8_t button) noexcept {
	switch ((MouseState::ButtonMap)button)
	{
	case MouseState::ButtonMap::Left:
		return "Left";
	case MouseState::ButtonMap::Wheel:
		return "Wheel";
	case MouseState::ButtonMap::Right:
		return "Right";
	case MouseState::ButtonMap::Mouse_3:
		return "Mouse 3";
	case MouseState::ButtonMap::Mouse_4:
		return "Mouse 4";
	case MouseState::ButtonMap::Mouse_5:
		return "Mouse 5";
	case MouseState::ButtonMap::Wheel_Up:
		return "Wheel up";
	case MouseState::ButtonMap::Wheel_Down:
		return "Wheel down";
	default:
		return std::string("Unsupported: ") + std::to_string(button);
	}
}

void render_key_list(const KeyboardState& ks) noexcept {
	static std::optional<uint8_t> key_selected;
	static bool render_idx{ false };
	const auto& list = ks.get_n_of_all_keys();

	std::vector<std::pair<std::uint8_t, size_t>> key_pair(list.size());
	for (size_t i = 0; i < list.size(); ++i) {
		key_pair[i] = { (std::uint8_t)i, list[i] };
	}

	std::sort(std::begin(key_pair), std::end(key_pair), [](auto a, auto b) {
		return a.second > b.second;
	});

	ImGui::Text("All keys");
	ImGui::SameLine();
	ImGui::Checkbox("Index", &render_idx);
	ImGui::Columns(render_idx ? 3 : 2, "key - n", false);
	for (uint8_t i = 0; i < key_pair.size(); ++i) {
		auto it = key_pair[i].first;
		auto x = key_pair[i].second;
		if (x == 0) continue;

		if (render_idx) {
			ImGui::Text("%u", it);
			ImGui::NextColumn();
		}

		bool selected = ImGui::Selectable(
			get_name_of_key(it).c_str(),
			key_selected && *key_selected == it,
			ImGuiSelectableFlags_SpanAllColumns
		);
		if (selected) {
			key_selected = it;
		}

		ImGui::NextColumn();
		ImGui::Text("%s", std::to_string(x).c_str());
		ImGui::NextColumn();
	}

	if (key_selected) {
		std::string window_data_title = "Data of: " + get_name_of_key(*key_selected);
		ImGui::Begin(window_data_title.c_str());
		ImGui::Text("Come back later !");
		ImGui::End();
	}
}

void render_keyboard_activity_timeline(const KeyboardState& ks) noexcept {
	return;
	static bool dirty{true};
	static int day_step{1};
	static std::vector<float> occ;

	if (ImGui::SliderInt("Day step", &day_step, 1, 31)) {
		dirty = true;
	}
	if (ks.key_entries.empty()) return;

	if (dirty) {
		auto time_start = ks.key_entries.front().timestamp;
		auto time_end = ks.key_entries.back().timestamp;
		auto step = (day_step * 3600 * 24);

		occ.resize(0);
		occ.resize(std::ceill((time_end - time_start) / step));

		for (size_t i = 0; i < ks.key_entries.size(); ++i) {
			auto& x = ks.key_entries[i];
			auto it = (x.timestamp - time_start) / step;
			occ[it]++;
		}

		dirty = false;
	}

	ImGui::PlotHistogram("", occ.data(), occ.size());
}

void render_mouse_list(const MouseState& ms) noexcept {
	static std::optional<uint8_t> mouse_selected;

	auto list = ms.buttons;

	ImGui::Columns(2, "button - n", false);
	for (uint8_t i = 0; i < ms.buttons.size(); ++i) {
		auto x = list[i];
		if (x == 0) continue;
		bool selected = ImGui::Selectable(
			get_name_of_mouse_button(i).c_str(),
			mouse_selected && *mouse_selected == i,
			ImGuiSelectableFlags_SpanAllColumns
		);
		if (selected) {
			mouse_selected = i;
		}

		ImGui::NextColumn();
		ImGui::Text("%s", std::to_string(x).c_str());
		ImGui::NextColumn();
	}
}

void render_mouse_plot(const MouseState& ms) noexcept {
	ms.cache.usage_plot.dirty |= ImGui::SliderSize(
		"Avg (s)", &ms.cache.usage_plot.rolling_average, 1, 100
	);
	ms.cache.usage_plot.dirty |= ImGui::SliderSize(
		"Resolution", &ms.cache.usage_plot.resolution, 10, 1000
	);

	ImPlot::SetNextPlotLimitsX(0, ms.cache.usage_plot.resolution);
	if (!ImPlot::BeginPlot("Mouse usage", "Time", "Usage")) return;
	defer { ImPlot::EndPlot(); };

	if (ms.cache.usage_plot.dirty && !ms.click_entries.empty()) {
		ms.cache.usage_plot.dirty = false;
		ms.cache.usage_plot.values.clear();

		auto range = ms.cache.usage_plot.rolling_average * 500'000;
		size_t n = 0;

		auto min = ms.click_entries.front().timestamp;
		auto max = ms.click_entries.front().timestamp;
		for (auto& x : ms.click_entries) {
			min = min > x.timestamp ? x.timestamp : min;
			max = max > x.timestamp ? max : x.timestamp;
		}
		auto u = max - min;

		for (size_t i = 0; i < ms.cache.usage_plot.resolution; ++i) {
			n = 0;

			auto t = i / (ms.cache.usage_plot.resolution - 1.f);
			for (auto& x : ms.click_entries) if (
				min + u * t -range < x.timestamp &&
				x.timestamp < min + u * t + range
			) n++;
			
			ms.cache.usage_plot.values.push_back(n / (2.f * range / 1'000'000.f));
		}
	}

	ImPlot::PlotLine(
		"Usage",
		ms.cache.usage_plot.values.data(),
		ms.cache.usage_plot.values.size()
	);
}


void render_display_stat(const MouseState& ms, const Display& d) noexcept {
	auto& it = ms.cache.n_keys[d.unique_hash_char];

	if (it.dirty) {
		size_t sum = 0;
		for (size_t i = 0; i < ms.click_entries.size(); ++i) {
			auto& x = ms.click_entries[i];

			if (x.timestamp < d.timestamp_start) continue;
			if (x.timestamp > d.timestamp_end) break;
			if (d.x > x.x || x.x > d.x + d.width) continue;
			if (d.y > x.y || x.y > d.y + d.height) continue;
			sum++;
		}

		it.v = sum;
		it.dirty = false;
	}

	auto name = d.unique_hash_char;
	if (*d.custom_name) name = d.custom_name;

	ImGui::Text("Click entry in %.32s: %zu", d.custom_name, it.v);
}
