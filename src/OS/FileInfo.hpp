#pragma once
#include <string>
#include <optional>
#include <filesystem>

extern std::optional<std::string> get_exe_description(
	const std::filesystem::path& path
) noexcept;