#include "Logs.hpp"

#include "Common.hpp"

#include "imgui.h"

void Logs::lock_and_write(const std::string& str) noexcept {
	std::lock_guard{ mutex };
	list.push_back(str);

	LogEntry new_entry;
	new_entry.message = str;
	new_entry.tag.push_back("OLD");
	entries.push_back(new_entry);
}
void Logs::lock_and_write(LogEntry entry) noexcept {
	std::lock_guard{ mutex };
	entries.push_back(entry);
}
void Logs::lock_and_write(ErrorDescription error) noexcept {
	std::lock_guard{ mutex };
	errors.push_back(error);
}

void Logs::lock_and_clear() noexcept {
	std::lock_guard{ mutex };
	list.clear();
	entries.clear();
}

void LogWindow::render(Logs& l) noexcept {
	if (!open) return;

	ImGui::Begin("Log", &open);
	defer{ ImGui::End(); };

	auto window_width = ImGui::GetWindowContentRegionWidth();
	ImGui::BeginChild("Text", { window_width / 2, 0 });

	ImGui::Text("Text: %u -----", l.entries.size());
	ImGui::Columns(2);
	for (size_t i = l.entries.size() - 1; i + 1 > 0; --i) {
		ImGui::PushID(i);
		defer{ ImGui::PopID(); };

		auto& x = l.entries[i];
		std::string line;

		if (!x.tag.empty()) {
			std::string tag_string = "[";
			for (auto t : x.tag) {
				tag_string += t + ", ";
			}
			tag_string.pop_back();
			tag_string.back() = ']';
			line += tag_string + ' ';
		}

		line += x.message;
		ImGui::Text(line.c_str());
		ImGui::NextColumn();
		if (ImGui::Button("X")) {
			l.entries.erase(std::begin(l.entries) + i);
		}
		ImGui::NextColumn();
	}
	ImGui::Columns(1);
	ImGui::EndChild();

	ImGui::SameLine();
	
	ImGui::BeginChild("Error", { window_width / 2, 0 });

	ImGui::Text("Errors: %u -----", l.errors.size());
	ImGui::Columns(3);
	for (size_t i = l.errors.size() - 1; i + 1 > 0; --i) {
		ImGui::PushID(i);
		defer{ ImGui::PopID(); };

		auto& x = l.errors[i];
		ImGui::Text(x.quick_desc.c_str());
		if (ImGui::IsItemHovered()) {
			ImGui::OpenPopup("Complete");
		}

		ImGui::NextColumn();
		switch (x.type) {
		case ErrorDescription::Type::FileIO:
			ImGui::Text("FileIO");
			break;
		case ErrorDescription::Type::Hook:
			ImGui::Text("Hook  ");
			break;
		case ErrorDescription::Type::Notify:
			ImGui::Text("Notify");
			break;
		case ErrorDescription::Type::Reg:
			ImGui::Text("Reg   ");
			break;
		default: assert(false);
		}
		ImGui::NextColumn();
		if (ImGui::Button("X")) {
			l.errors.erase(std::begin(l.errors) + i);
		}
		ImGui::NextColumn();
		if (ImGui::BeginPopup("Complete")) {
			ImGui::PushItemWidth(200);
			defer{
				ImGui::PopItemWidth();
				ImGui::EndPopup();
			};

			ImGui::Text("Location %s.\n%s", x.location.c_str(), x.message.c_str());
		}
	}
	ImGui::Columns(1);
	ImGui::EndChild();
}

