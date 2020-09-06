#include "Event.hpp"

#include "imgui.h"
#include "Common.hpp"
#include "Logs.hpp"

#include "file.hpp"
#include "TimeInfo.hpp"

#include "xstd.hpp"

#include <set>
#include <array>
#include <vector>

struct Version_0 {
	static constexpr size_t File_Signature_Offset    = 0;
	static constexpr size_t Version_Offset           = 4;
	static constexpr size_t Size_Table_Offset      = 5;
	static constexpr size_t Size_Table_Size        = 4;
};

std::optional<EventState> version0_read(const std::vector<std::byte>& bytes) noexcept;
bool version0_write(const EventState& state, std::filesystem::path path) noexcept;

std::optional<EventState> version0_read(const std::vector<std::byte>& bytes) noexcept {
	size_t it = Version_0::Size_Table_Offset + Version_0::Size_Table_Size;
	if (bytes.size() < it) {
		ErrorDescription error;
		error.location = "version0_read:EventState";
		error.quick_desc = "The event file is too short. It's ill-formed.";
		error.message = "The file is: " + std::to_string(bytes.size()) + " when it should be at"
			"least" + std::to_string(it) + " bytes long.";
		error.type = ErrorDescription::Type::FileIO;
		logs.lock_and_write(error);
		return std::nullopt;
	}

	EventState es;
	es.apps_usages.resize(read_uint32(bytes, Version_0::Size_Table_Offset + 0));

	size_t n = it + es.apps_usages.size() * AppUsage::Byte_Size;

	if (bytes.size() < n) {
		ErrorDescription error;
		error.location = "version0_read:EventState";
		error.quick_desc = "The event file is too short. It's ill-formed.";
		error.message = "The file is: " + std::to_string(bytes.size()) + " when it should be at"
			"least" + std::to_string(n) + " bytes long.";
		error.type = ErrorDescription::Type::FileIO;
		logs.lock_and_write(error);
		return std::nullopt;
	}

	for (size_t i = 0; i < es.apps_usages.size(); ++i) {
		for (size_t j = 0; j < AppUsage::Max_String_Size; ++j, ++it)
			es.apps_usages[i].exe_name[j] = read_uint8(bytes, it);
		for (size_t j = 0; j < AppUsage::Max_String_Size; ++j, ++it)
			es.apps_usages[i].doc_name[j] = read_uint8(bytes, it);
		es.apps_usages[i].timestamp_start = read_uint64(bytes, it + 0);
		es.apps_usages[i].timestamp_end = read_uint64(bytes, it + 8);
		it += 16;
	}
	return es;
}

bool version0_write(const EventState& state, std::filesystem::path path) noexcept {
	std::vector<std::byte> bytes;
	insert_uint32(bytes, Event_File_Signature);
	insert_uint8(bytes, 0);

	insert_uint32(bytes, state.apps_usages.size());

	for (auto& x : state.apps_usages) {
		for (auto& c : x.exe_name) insert_uint8(bytes, c);
		for (auto& c : x.doc_name) insert_uint8(bytes, c);
		insert_uint64(bytes, x.timestamp_start);
		insert_uint64(bytes, x.timestamp_end);
	}

	return file_overwrite_byte(bytes, path) == 0;
}

std::optional<EventState> EventState::load_from_file(
	std::filesystem::path path
) noexcept {
	const auto& opt_bytes = file_read_byte(path);
	if (!opt_bytes) {
		ErrorDescription error;
		error.location = "EventState::load_from_file";
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
		error.location = "EventState::load_from_file";
		error.message = "The file is ill formed it's too short to contain the magic number and the"
		" version number";
		error.quick_desc = "File too short.";
		error.type = ErrorDescription::Type::FileIO;

		logs.lock_and_write(error);

		return std::nullopt;
	}

	auto signature = read_uint32(bytes, it);
	if (signature != Event_File_Signature) {
		ErrorDescription error;
		error.location = "EventState::load_from_file";
		error.quick_desc = "The event file save has been corrupted.";
		error.message = "The file signature is: " + std::string((char*)&signature, 4) +
			" when it should be " + std::string((char*)&Event_File_Signature, 4);
		error.type = ErrorDescription::Type::FileIO;
		logs.lock_and_write(error);

		return std::nullopt;
	}
	it += 4;
	auto version_number = read_uint8(bytes, it);

	switch (version_number) {
	case 0:
		return version0_read(bytes);
	default: {
		ErrorDescription error;
		error.location = "EventState::load_from_file";
		error.quick_desc = "Unknown version number: " + std::to_string(version_number);
		error.type = ErrorDescription::Type::FileIO;
		logs.lock_and_write(error);
		return std::nullopt;
	}
	}
}

bool EventState::save_to_file(std::filesystem::path path) noexcept {
	return version0_write(*this, path);
}

void EventState::register_event(AppUsage event) noexcept {
	apps_usages.push_back(event);
	modifications_since_save++;

	cache.dirty = true;

	check_resave();
}

void EventState::check_resave() noexcept {
	if (modifications_since_save >= Save_Every_Mod) {
		//>TODO: do that in another thread it's kind of a dick move to block inputs...
		if (!save_to_file(get_user_data_path() / App_Data_Dir_Name / Default_Path)) {
			logs.lock_and_write("Can't save incremental change...");
		}
		// we reset it anyway to not spam File IO.
		modifications_since_save = 0;
	}
}


void EventWindow::render(std::optional<EventState>& state) noexcept {
	ImGui::Begin("Event");
	defer { ImGui::End(); };

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
		ImGui::Text("There was a problem loading event.mto, you can try to reset the file.");
		return;
	}

	ImGui::SameLine();
	if (ImGui::Button("Save")) save = true;
	ImGui::SameLine();
	if (ImGui::Button("Reload")) reload = true;
	ImGui::SameLine();
	if (ImGui::Button("Kill hook")) {
		__declspec(dllimport) extern bool uninstall_hook();
		uninstall_hook();
	}

	ImGui::Separator();
	state->cache.dirty |= ImGui::Checkbox("Sort less", &sort_less);
	ImGui::Separator();


	if (state->cache.dirty) {
		state->cache.doc_to_time.clear();
		state->cache.exe_to_time.clear();
		state->cache.exe_to_docs.clear();
		for (auto& x : state->apps_usages) {
			state->cache.doc_to_time[x.doc_name] +=
				x.timestamp_end - x.timestamp_start;
			state->cache.exe_to_time[x.exe_name] +=
				x.timestamp_end - x.timestamp_start;
			state->cache.exe_to_docs[x.exe_name].insert(x.doc_name);
		}

		auto cmp = [&](const auto& a, const auto& b) { return sort_less ? (a < b) : (a > b); };

		state->cache.exe_to_time_sorted.clear();
		state->cache.exe_to_time_sorted.reserve(state->cache.exe_to_time.size());
		for (auto& x : state->cache.exe_to_time) {
			auto& name = x.first;
			state->cache.exe_to_time_sorted.push_back(name);

			auto& vec = state->cache.exe_to_docs_sorted[name];
			vec.clear();
			vec.reserve(state->cache.exe_to_docs[name].size());
			for (auto& y : state->cache.exe_to_docs[name]) vec.push_back(y);
			std::sort(
				std::begin(vec),
				std::end(vec),
				[&](auto& a, auto& b) {
					return cmp(
						state->cache.doc_to_time[a],
						state->cache.doc_to_time[b]
					);
				}
			);
		}

		std::sort(
			std::begin(state->cache.exe_to_time_sorted),
			std::end(state->cache.exe_to_time_sorted),
			[&](auto& a, auto& b) {
				return cmp(state->cache.exe_to_time[a], state->cache.exe_to_time[b]);
			}
		);

		state->cache.dirty = false;
	}

	ImGui::Text("N %zu", state->apps_usages.size());

	ImGui::Columns(2);



	for (auto& name : state->cache.exe_to_time_sorted) {
		bool open = ImGui::TreeNode(name.data(), "%s", name.data());

		ImGui::NextColumn();

		ImGui::Text(
			"% 25.3lf for % 3d documents.",
			state->cache.exe_to_time[name] / 1'000'000.0,
			(int)state->cache.exe_to_docs[name].size()
		);

		ImGui::NextColumn();

		if (open) {
			defer { ImGui::TreePop(); };

			for (auto& doc : state->cache.exe_to_docs_sorted[name]) {
				ImGuiTreeNodeFlags flags =
					ImGuiTreeNodeFlags_Leaf |
					ImGuiTreeNodeFlags_NoTreePushOnOpen |
					ImGuiTreeNodeFlags_Bullet;
				ImGui::TreeNodeEx(doc.data(), flags, "%s", doc.data());
				ImGui::NextColumn();
				ImGui::Text(
					"% 25.3lf", state->cache.doc_to_time[doc] / 1'000'000.0
				);
				ImGui::NextColumn();
			}
		}
	}
	ImGui::Columns(1);
}

bool EventState::reset_everything() noexcept {
	*this = {};

	return save_to_file(get_user_data_path() / App_Data_Dir_Name / Default_Path);
}
