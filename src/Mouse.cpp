#include "Mouse.hpp"

#include <cassert>
#include <unordered_set>

#include "imgui.h"

#include "file.hpp"
#include "Common.hpp"
#include "Logs.hpp"

#include "render_stats.hpp"

const std::filesystem::path MouseState::Default_Path{ "mouse.mto" };
const size_t MouseState::Save_Every_Mod{ 50 };
struct Version_0 {
	static constexpr size_t File_Signature_Offset     = 0;
	static constexpr size_t Verison_Offset            = 4;
	static constexpr size_t Click_Occurence_Offset    = 5;
	static constexpr size_t Click_Occurence_Size      = 4 * (MouseState::N_Button_Supported + 2);
	static constexpr size_t Click_Entry_Size_Offset   = 5 + Click_Occurence_Size;
	static constexpr size_t Display_Entry_Size_Offset = Click_Entry_Size_Offset + 4;
};

std::optional<MouseState> version0_read(const std::vector<std::byte>& bytes, bool strict) noexcept;
std::optional<MouseState> version1_read(const std::vector<std::byte>& bytes, bool strict) noexcept;
bool version0_write(const MouseState& state, const std::filesystem::path& path) noexcept;

std::optional<MouseState> MouseState::load_from_file(
	const std::filesystem::path& path, bool strict
) noexcept {
	const auto& opt_bytes = file_read_byte(path);
	if (!opt_bytes) {
		ErrorDescription error;
		error.location = "MouseState::load_from_file";
		error.message = "Can't open the file.";
		error.quick_desc = "Can't open the file.";
		error.type = ErrorDescription::Type::FileIO;

		logs.lock_and_write(error);

		return std::nullopt;
	}
	const auto& bytes = *opt_bytes;

	size_t it{ 0 };
	if (bytes.size() < 5) {
		ErrorDescription error;
		error.location = "MouseState::load_from_file";
		error.message = "The file is ill formed it's too short to contain the magic number and the "
		"version number";
		error.quick_desc = "File too short.";
		error.type = ErrorDescription::Type::FileIO;

		logs.lock_and_write(error);

		return std::nullopt;
	}

	auto signature = read_uint32(bytes, it);
	if (signature != Mouse_File_Signature) {
		ErrorDescription error;
		error.location = "MouseState::load_from_file";
		error.quick_desc = "The mouse file save has been corrupted.";
		error.message = "The file signature is: " + std::string((char*)&signature, 4) +
			" when it should be " + std::string((char*)&Mouse_File_Signature, 4);
		error.type = ErrorDescription::Type::FileIO;
		logs.lock_and_write(error);

		return std::nullopt;
	}
	it += 4;
	auto version_number = read_uint8(bytes, it);

	switch (version_number) {
	case 0:
		return version0_read(bytes, strict);
	case 1:
		return version1_read(bytes, strict);
	default: {
		ErrorDescription error;
		error.location = "MouseState::load_from_file";
		error.quick_desc = "Unknown version number: " + std::to_string(version_number);
		error.type = ErrorDescription::Type::FileIO;
		logs.lock_and_write(error);
		return std::nullopt;
	}
	}
}

bool MouseState::save_to_file(const std::filesystem::path& path) noexcept {
	return version0_write(*this, path);
}

size_t MouseState::increment_button(ClickEntry click) noexcept {
	for (auto& d : display_entries) {
		if (click.timestamp > d.timestamp_end) continue;
		if (d.x > click.x || click.x > d.x + d.width) continue;
		if (d.y > click.y || click.y > d.y + d.height) continue;

		cache.n_keys[d.unique_hash_char].v++;
	}

	click_entries.push_back(click);

	++modifications_since_save;
	++buttons[click.button_code];


	if (modifications_since_save >= Save_Every_Mod) {
		//>TODO: do that in another thread it's kind of a dick move to block inputs...
		if (!save_to_file(get_user_data_path() / App_Data_Dir_Name / Default_Path)) {
			logs.lock_and_write("Can't save incremental change...");
		}
		// we reset it anyway to not spam File IO.
		modifications_since_save = 0;
	}

	return buttons[click.button_code];
}

bool MouseState::reset_everything() noexcept {
	*this = MouseState{};
	version_number = 0;

	return save_to_file(get_user_data_path() / App_Data_Dir_Name / Default_Path);
}

void MouseWindow::render(std::optional<MouseState>& state) noexcept {
	auto full_path = get_app_data_path() / MouseState::Default_Path;

	ImGui::Begin("Mouse");
	defer{ ImGui::End(); };

	if (reset_time_start == 0 && ImGui::Button("Reset")) {
		reset_time_start = time(nullptr);
	}
	else if (reset_time_start > 0) {
		if (ImGui::Button("Sure ?")) {
			reset = true;
			reset_time_start = 0;
		}

		ImGui::SameLine();

		auto time_end = time(nullptr);
		ImGui::Text("%d", (int)(Reset_Button_Time - (time_end - reset_time_start)));
		if (time_end - reset_time_start + 1 > Reset_Button_Time) {
			reset_time_start = 0;
		}
	}
	if (!state) {
		ImGui::Checkbox("Strict", &strict);
		ImGui::SameLine();
		if (ImGui::Button("Retry")) {
			reload = true;
		}
		ImGui::Text(
			"There was a problem loading mouse.mto, you can try to reset the mouse config."
		);
		return;
	}
	ImGui::SameLine();
	if (ImGui::Button("Save")) {
		save = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Reload")) {
		reload = true;
	}
	ImGui::Separator();
	ImGui::Text("Display");
	ImGui::SameLine();

	if (ImGui::CollapsingHeader("Monitors")) {
		for (uint32_t i = 0; i < state->display_entries.size(); ++i) {
			auto& x = state->display_entries[i];
			char* name = x.unique_hash_char;
			if (*x.custom_name) name = x.custom_name;
			ImGui::PushID(i);
			defer { ImGui::PopID(); };

			if (display_list_focused && display_list_selected == i) {
				if (ImGui::InputText(
					"",
					x.custom_name,
					Display::Custom_Name_Size - 1,
					ImGuiInputTextFlags_EnterReturnsTrue
				)) display_list_focused = false;
				ImGui::SameLine();
				if (ImGui::Button("X")) {
					state->remove_display(i);
					i--;
				}
			}
			else if (ImGui::Selectable(name, false)) {
				display_list_selected = i;
				display_list_focused = true;
			}
		}
	}

	ImGui::Separator();
	ImGui::Checkbox("Buttons list", &render_buttons_list_checkbox);

	ImGui::Columns(2);
	ImGui::BeginChild("list");
	if (render_buttons_list_checkbox) {
		render_mouse_list(*state);
	}
	ImGui::EndChild();
	ImGui::NextColumn();
	ImGui::BeginChild("display");
	if (!state->display_entries.empty())
		render_display_stat(*state, state->display_entries[display_list_selected]);
	ImGui::EndChild();
	ImGui::Columns(1);

	ImGui::Separator();

	render_mouse_plot(*state);
}

std::optional<MouseState> version0_read(
	const std::vector<std::byte>& bytes, bool strict
) noexcept {
	size_t it = 5; // we start after the version byte and the signature bytes(4).

	if (bytes.size() < Version_0::Display_Entry_Size_Offset) {
		ErrorDescription error;
		error.location = "version0_read";
		error.quick_desc = "The mouse file is too short. It's ill-formed.";
		error.message = "The file is: " + std::to_string(bytes.size()) + " when it should be at"
			"least" + std::to_string(Version_0::Display_Entry_Size_Offset) + " bytes long.";
		error.type = ErrorDescription::Type::FileIO;
		logs.lock_and_write(error);
		return std::nullopt;
	}

	MouseState ms;

	for (size_t i = 0; it < Version_0::Click_Occurence_Size + Version_0::Click_Occurence_Offset; it += 4, i++) {
		ms.buttons[i] = read_uint32(bytes, it);
	}

	auto click_entries_size = read_uint32(bytes, it);
	ms.click_entries.reserve(click_entries_size);

	it += 4;

	auto display_entries_size = read_uint32(bytes, it);
	ms.display_entries.reserve(display_entries_size);

	it += 4;

	auto file_size_verification =
		Version_0::Display_Entry_Size_Offset +
		13 * click_entries_size +
		88 * display_entries_size;

	if (bytes.size() < file_size_verification) {
		ErrorDescription error;
		error.location = "version0_read";
		error.quick_desc = "The mouse file is too short. It's ill-formed.";
		error.message = "The file is: " + std::to_string(bytes.size()) + " when it should be at"
			"least" + std::to_string(file_size_verification) + " bytes long." +
			" The error is probably in the click and/or display entry list.";
		error.type = ErrorDescription::Type::FileIO;
		logs.lock_and_write(error);

		if (strict) return std::nullopt;
	}

	for (
		size_t i = 0;
		i < display_entries_size && it + Display::Byte_Size < 8 + bytes.size();
		++i, it += (Display::Byte_Size - 8)
	) {
		size_t off = 0;
		Display d;

		d.width = read_uint32(bytes, it + off); off += sizeof(d.width);
		d.height = read_uint32(bytes, it + off); off += sizeof(d.height);
		d.x = read_uint32(bytes, it + off); off += sizeof(d.x);
		d.y = read_uint32(bytes, it + off); off += sizeof(d.y);
		for (size_t j = 0; j < Display::Unique_Hash_Size; ++j, ++off) {
			d.unique_hash_char[j] = read_uint8(bytes, it + off);
		}
		for (size_t j = 0; j < Display::Custom_Name_Size; ++j, ++off) {
			d.custom_name[j] = read_uint8(bytes, it + off);
		}
		d.timestamp_start = read_uint32(bytes, it + off); off += sizeof(std::uint32_t);
		d.timestamp_end = read_uint32(bytes, it + off); off += sizeof(std::uint32_t);

		ms.display_entries.push_back(d);
	}

	for (
		size_t i = 0;
		i < click_entries_size && it + ClickEntry::Byte_Size < bytes.size() + 4;
		++i, it += (ClickEntry::Byte_Size - 4)
	) {
		ClickEntry click_entry;

		click_entry.button_code = read_uint8(bytes, it);
		click_entry.x = read_uint32(bytes, it + 1);
		click_entry.y = read_uint32(bytes, it + 5);
		click_entry.timestamp = read_uint32(bytes, it + 9);

		ms.click_entries.push_back(click_entry);
	}

	return ms;
}

std::optional<MouseState> version1_read(
	const std::vector<std::byte>& bytes, bool strict
) noexcept {
	size_t it = 5; // we start after the version byte and the signature bytes(4).

	if (bytes.size() < Version_0::Display_Entry_Size_Offset) {
		ErrorDescription error;
		error.location = "version0_read";
		error.quick_desc = "The mouse file is too short. It's ill-formed.";
		error.message = "The file is: " + std::to_string(bytes.size()) + " when it should be at"
			"least" + std::to_string(Version_0::Display_Entry_Size_Offset) + " bytes long.";
		error.type = ErrorDescription::Type::FileIO;
		logs.lock_and_write(error);
		return std::nullopt;
	}

	MouseState ms;

	for (size_t i = 0; it < Version_0::Click_Occurence_Size + Version_0::Click_Occurence_Offset; it += 4, i++) {
		ms.buttons[i] = read_uint32(bytes, it);
	}

	auto click_entries_size = read_uint32(bytes, it);
	ms.click_entries.reserve(click_entries_size);

	it += 4;

	auto display_entries_size = read_uint32(bytes, it);
	ms.display_entries.reserve(display_entries_size);

	it += 4;

	auto file_size_verification =
		Version_0::Display_Entry_Size_Offset +
		ClickEntry::Byte_Size * click_entries_size +
		Display::Byte_Size * display_entries_size;

	if (bytes.size() < file_size_verification) {
		ErrorDescription error;
		error.location = "version0_read";
		error.quick_desc = "The mouse file is too short. It's ill-formed.";
		error.message = "The file is: " + std::to_string(bytes.size()) + " when it should be at"
			"least" + std::to_string(file_size_verification) + " bytes long." +
			" The error is probably in the click and/or display entry list.";
		error.type = ErrorDescription::Type::FileIO;
		logs.lock_and_write(error);

		if (strict) return std::nullopt;
	}

	for (
		size_t i = 0;
		i < display_entries_size && it + Display::Byte_Size < bytes.size();
		++i, it += Display::Byte_Size
	) {
		size_t off = 0;
		Display d;

		d.width = read_uint32(bytes, it + off); off += sizeof(d.width);
		d.height = read_uint32(bytes, it + off); off += sizeof(d.height);
		d.x = read_uint32(bytes, it + off); off += sizeof(d.x);
		d.y = read_uint32(bytes, it + off); off += sizeof(d.y);
		for (size_t j = 0; j < Display::Unique_Hash_Size; ++j, ++off) {
			d.unique_hash_char[j] = read_uint8(bytes, it + off);
		}
		for (size_t j = 0; j < Display::Custom_Name_Size; ++j, ++off) {
			d.custom_name[j] = read_uint8(bytes, it + off);
		}
		d.timestamp_start = read_uint64(bytes, it + off); off += sizeof(d.timestamp_start);
		d.timestamp_end = read_uint64(bytes, it + off); off += sizeof(d.timestamp_end);

		ms.display_entries.push_back(d);
	}

	for (
		size_t i = 0;
		i < click_entries_size && it + ClickEntry::Byte_Size < bytes.size();
		++i, it += ClickEntry::Byte_Size
	) {
		ClickEntry click_entry;

		click_entry.button_code = read_uint8(bytes, it);
		click_entry.x = read_uint32(bytes, it + 1);
		click_entry.y = read_uint32(bytes, it + 5);
		click_entry.timestamp = read_uint64(bytes, it + 9);

		ms.click_entries.push_back(click_entry);
	}

	return ms;
}

bool version0_write(const MouseState& state, const std::filesystem::path& path) noexcept {
	std::vector<std::byte> bytes;
	insert_uint32(bytes, Mouse_File_Signature);
	insert_uint8(bytes, 1);

	for (auto& x : state.buttons) {
		insert_uint32(bytes, x);
	}
	
	insert_uint32(bytes, state.click_entries.size());
	insert_uint32(bytes, state.display_entries.size());

	for (auto& x : state.display_entries) {
		insert_uint32(bytes, x.width);
		insert_uint32(bytes, x.height);
		insert_uint32(bytes, x.x);
		insert_uint32(bytes, x.y);
		for (int i = 0; i < Display::Unique_Hash_Size; ++i) {
			insert_uint8(bytes, x.unique_hash_char[i]);
		}
		for (int i = 0; i < Display::Custom_Name_Size; ++i) {
			insert_uint8(bytes, x.custom_name[i]);
		}
		insert_uint64(bytes, x.timestamp_start);
		insert_uint64(bytes, x.timestamp_end);
	}

	for (auto& x : state.click_entries) {
		insert_uint8(bytes, x.button_code);
		insert_uint32(bytes, x.x);
		insert_uint32(bytes, x.y);
		insert_uint64(bytes, x.timestamp);
	}

	return file_overwrite_byte(bytes, path) == 0;
}

void MouseState::remove_display(size_t display_idx) noexcept {
	cache.n_keys.erase(display_entries[display_idx].unique_hash_char);
	display_entries.erase(std::begin(display_entries) + display_idx);
}
