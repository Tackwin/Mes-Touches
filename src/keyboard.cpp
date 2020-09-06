#include "keyboard.hpp"
#include <cassert>

#include "imgui.h"

#include "file.hpp"
#include "TimeInfo.hpp"
#include "ErrorCode.hpp"
#include "Common.hpp"
#include "Logs.hpp"
#include "render_stats.hpp"

struct Version_0 {
	static constexpr size_t File_Signature_Offset                                = 0;
	static constexpr size_t Version_Offset                                       = 4;
	static constexpr size_t Key_Entry_Occurence_Offset                           = 5;
	static constexpr size_t Key_Entry_Occurence_Size                             = 255;
	static constexpr size_t Key_Entry_List_Size_Offset                           = 5 + 255 * 4;
	static constexpr size_t Key_Entry_List_Offset                                = 5 + 255 * 4 + 4;
};

std::optional<KeyboardState> version0_read(const std::vector<std::byte>& bytes) noexcept;
std::optional<KeyboardState> version1_read(const std::vector<std::byte>& bytes) noexcept;
bool version1_write(const KeyboardState& state, const std::filesystem::path& path) noexcept;

// ughhhh constexpr as a first class cityzen in this langage can not happen soon enough.
extern const std::filesystem::path Default_Keyboard_Path{ "keyboard.mto" };

std::optional<std::vector<std::byte>> get_raw_keyboard_data(std::filesystem::path path) noexcept {
	std::vector<std::byte> data;

	auto opt = file_read_byte(path);
	if (!opt) return std::nullopt;
	data.swap(*opt);

	return data;
}
void set_raw_keyboard_data(const std::vector<std::byte>& bytes) noexcept {
	auto full_path =
		get_user_data_path() / App_Data_Dir_Name / Default_Keyboard_Path;
	if (!std::filesystem::is_regular_file(full_path)) {
		auto err = file_overwrite_byte(bytes, full_path);
		if (err) {
			ErrorDescription error;
			error.location = "set_raw_keyboard_data";
			error.quick_desc = "Couldn't overwrite byte to the keyboard save file.";
			error.message = format_error_code(err);
			error.type = ErrorDescription::Type::FileIO;
			logs.lock_and_write(error);
		}
		return;
	}
	//>TODO: error handling.
	auto err = file_replace_byte(full_path, bytes, 0);
	if (err) {
		logs.lock_and_write("set_raw_keyboard_data, file_replace_byte: " + std::to_string(err));
	}
}

std::optional<KeyboardState> KeyboardState::load_from_file(std::filesystem::path path) noexcept {
	auto opt_bytes = get_raw_keyboard_data(path);
	if (!opt_bytes) {
		ErrorDescription error;
		error.location = "KeyboardState::load_from_file";
		error.message = "Can't read the keyboard save file from: " + path.generic_string();
		error.quick_desc = "Reading the keyboard file save.";
		error.type = ErrorDescription::Type::FileIO;
		logs.lock_and_write(error);

		return std::nullopt;
	}
	auto& bytes = *opt_bytes;

	size_t it = 0;
	if (bytes.size() < 5) {
		ErrorDescription error;
		error.location = "KeyboardState::load_from_file";
		error.quick_desc = "The keyboard file save is too small to be well formed.";
		error.message = "The file is: " + std::to_string(bytes.size()) + " when it should be at"
			"least 5 bytes.";
		error.type = ErrorDescription::Type::FileIO;
		logs.lock_and_write(error);
		
		return std::nullopt;
	}
	auto signature = read_uint32(bytes, it);
	if (signature != Keyboard_File_Signature) {
		ErrorDescription error;
		error.location = "KeyboardState::load_from_file";
		error.quick_desc = "The keyboard file save has been corrupted.";
		error.message = "The file signature is: " + std::string((char*)&signature, 4) +
			" when it should be " + std::string((char*)&Keyboard_File_Signature, 4);
		error.type = ErrorDescription::Type::FileIO;
		logs.lock_and_write(error);

		return std::nullopt;
	}
	it += 4;
	auto version_number = read_uint8(bytes, it);

	switch (version_number) {
	case 0:
		return version0_read(bytes);
	case 1:
		return version1_read(bytes);
	default: {
		ErrorDescription error;
		error.location = "KeyboardState::load_from_file";
		error.quick_desc = "The keyboard file save's version is unsopported.";
		error.message = "The file version is: " + std::to_string(version_number);
		error.type = ErrorDescription::Type::FileIO;
		logs.lock_and_write(error);
		return std::nullopt;
	}
	}
}

[[nodiscard]] bool KeyboardState::save_to_file(std::filesystem::path path) const noexcept {
	return version1_write(*this, path) == 0;
}

std::array<size_t, 0xff> KeyboardState::get_n_of_all_keys() const noexcept {
	return key_times;
}

bool KeyboardState::reset_everything() noexcept {
	modifications_since_save = 0;
	version_number = 0;
	key_times = {};

	std::vector<std::byte> bytes;
	insert_uint32(bytes, Keyboard_File_Signature);
	insert_uint8(bytes, version_number);
	for (size_t i = 0; i < 255; ++i) {
		insert_uint32(bytes, key_times[i]);
	}
	insert_uint32(bytes, 0);

	auto full_path = get_app_data_path() / Default_Keyboard_Path;
	if (auto err = file_overwrite_byte(bytes, full_path); err) {
		ErrorDescription error;
		error.location = "KeyboardState::reset_everything";
		error.quick_desc = "Couldn't overwrite byte to the keyboard save file.";
		error.message = format_errno(err);
		error.type = ErrorDescription::Type::FileIO;
		logs.lock_and_write(error);
		return false;
	}
	return true;
}

void KeyboardState::increment_key(KeyEntry key_entry) noexcept {
	key_entries.push_back(key_entry);
	++modifications_since_save;
	++key_times[key_entry.key_code];

	if (modifications_since_save >= Keyboard_Save_Every_Mod) {
		if (save_to_file(get_app_data_path() / Default_Keyboard_Path)) {
			modifications_since_save = 0;
		}
		else {
			logs.lock_and_write(
				"KeyboardState::increment_key."
				"Error when trying to auto save after n modifications."
			);
		}
	}
}

void KeyboardWindow::render(std::optional<KeyboardState>& state) noexcept {
	auto full_path = get_app_data_path() / Default_Keyboard_Path;

	ImGui::Begin("Keyboard");
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
		ImGui::Text(
			"There was a problem loading keyboard.mto, you can try to reset the keyboard config."
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
	ImGui::Checkbox("key list", &render_key_list_checkbox);

	render_keyboard_activity_timeline(*state);

	if (render_key_list_checkbox) {
		render_key_list(*state);
	}
}

std::optional<KeyboardState> version1_read(const std::vector<std::byte>& bytes) noexcept {
	size_t it = 5; // we start after the version byte and the signature bytes(4).

	if (bytes.size() < it + (1 + 255) * 4) {
		ErrorDescription error;
		error.location = "version0_read";
		error.quick_desc = "The keyboard file save is too small to be well formed.";
		error.message = "The file is: " + std::to_string(bytes.size()) + " when it should be at"
			"least 5 + (1 + 255) * 4bytes.";
		error.type = ErrorDescription::Type::FileIO;
		logs.lock_and_write(error);

		return std::nullopt;
	}

	KeyboardState ks;

	for (size_t i = 0; i < 255; ++i) {
		ks.key_times[i] = read_uint32(bytes, it);
		it += 4;
	}

	// The next uint32_t is the size of the list of KeyEntries
	auto key_entries_size = read_uint32(bytes, it);
	ks.key_entries.reserve(key_entries_size);
	
	it += 4;

	if (bytes.size() < it + KeyEntry::Packed_Size * key_entries_size) {
		ErrorDescription error;
		error.location = "version0_read";
		error.quick_desc = "The keyboard file save is too small to be well formed.";
		error.message = "The file is: " + std::to_string(bytes.size()) + " long when it should be"
			" at least" + std::to_string(it + KeyEntry::Packed_Size * key_entries_size) + " bytes."
			"\nSince the key entry list size's is: " + std::to_string(key_entries_size);
		error.type = ErrorDescription::Type::FileIO;
		logs.lock_and_write(error);

		return std::nullopt;
	}

	for (size_t i = it; i < it + KeyEntry::Packed_Size * key_entries_size; i += 9) {
		KeyEntry entry;
		entry.key_code = read_uint8(bytes, i);
		entry.timestamp = read_uint64(bytes, i + 1);
		ks.key_entries.push_back(entry);
	}

	return ks;
}

std::optional<KeyboardState> version0_read(const std::vector<std::byte>& bytes) noexcept {
	size_t it = 5; // we start after the version byte and the signature bytes(4).

	if (bytes.size() < it + (1 + 255) * 4) {
		ErrorDescription error;
		error.location = "version0_read";
		error.quick_desc = "The keyboard file save is too small to be well formed.";
		error.message = "The file is: " + std::to_string(bytes.size()) + " when it should be at"
			"least 5 + (1 + 255) * 4bytes.";
		error.type = ErrorDescription::Type::FileIO;
		logs.lock_and_write(error);

		return std::nullopt;
	}

	KeyboardState ks;

	for (size_t i = 0; i < 255; ++i) {
		ks.key_times[i] = read_uint32(bytes, it);
		it += 4;
	}

	// The next uint32_t is the size of the list of KeyEntries
	auto key_entries_size = read_uint32(bytes, it);
	ks.key_entries.reserve(key_entries_size);
	
	it += 4;

	if (bytes.size() < it + 5 * key_entries_size) {
		ErrorDescription error;
		error.location = "version0_read";
		error.quick_desc = "The keyboard file save is too small to be well formed.";
		error.message = "The file is: " + std::to_string(bytes.size()) + " long when it should be"
			" at least" + std::to_string(it + 5 * key_entries_size) + " bytes."
			"\nSince the key entry list size's is: " + std::to_string(key_entries_size);
		error.type = ErrorDescription::Type::FileIO;
		logs.lock_and_write(error);

		return std::nullopt;
	}

	for (size_t i = it; i < it + 5 * key_entries_size; i += 5) {
		KeyEntry entry;
		entry.key_code = read_uint8(bytes, i);
		entry.timestamp = (uint64_t)read_uint32(bytes, i + 1);
		ks.key_entries.push_back(entry);
	}

	return ks;
}

bool version1_write(const KeyboardState& state, const std::filesystem::path& path) noexcept {
	std::vector<std::byte> bytes;

	insert_uint32(bytes, Keyboard_File_Signature);
	insert_uint8(bytes, 1);

	for (auto& x : state.key_times) {
		insert_uint32(bytes, x);
	}

	insert_uint32(bytes, state.key_entries.size());

	for (auto& x : state.key_entries) {
		insert_uint8(bytes, x.key_code);
		insert_uint64(bytes, x.timestamp);
	}

	return file_overwrite_byte(bytes, path);
}

void KeyboardState::repair() noexcept {
	if (key_entries.empty()) return;

	auto first = key_entries.front().timestamp;

	key_entries.erase(
		std::remove_if(std::begin(key_entries), std::end(key_entries), [&](auto x) {
			return x.timestamp < first;
		}),
		std::end(key_entries)
	);
}
