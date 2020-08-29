#pragma once
#include <cstddef>
#include <vector>
#include <filesystem>
#include <optional>
#include <type_traits>

[[nodiscard]] extern std::filesystem::path get_user_data_path() noexcept;

[[nodiscard]] extern std::optional<std::vector<std::byte>>
file_read_byte(const std::filesystem::path& path) noexcept;

[[nodiscard]] extern int
file_write_byte(const std::vector<std::byte>& bytes, const std::filesystem::path& path) noexcept;

[[nodiscard]] extern int file_overwrite_byte(
	const std::vector<std::byte>& bytes, const std::filesystem::path& path
) noexcept;

[[nodiscard]] extern std::optional<uint32_t>
file_read_uint32_t(const std::filesystem::path& path, size_t offset) noexcept;

[[nodiscard]] extern int
file_write_integer(const std::filesystem::path& path, uint32_t x, size_t offset) noexcept;

[[nodiscard]] extern int file_insert_byte(
	const std::filesystem::path& path, const std::vector<std::byte>& x, size_t offset
) noexcept;

[[nodiscard]] extern int file_replace_byte(
	const std::filesystem::path& path, const std::vector<std::byte>& x, size_t offset
) noexcept;
[[nodiscard]] extern int file_replace_uint32_t(
	const std::filesystem::path& path, uint32_t x, size_t offset
) noexcept;

[[nodiscard]] extern std::optional<uint64_t>
get_file_length(const std::filesystem::path& path) noexcept;
