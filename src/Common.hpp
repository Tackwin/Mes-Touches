#pragma once
#include <filesystem>
#include <vector>
#include "Logs.hpp"

namespace details {
	template<typename Callable>
	struct Defer {
		~Defer() noexcept { todo(); }
		Defer(Callable todo) noexcept : todo(todo) {};
	private:
		Callable todo;
	};
};

#define defer details::Defer _CONCAT(defer_, __COUNTER__) = [&]
#define BEG(x) std::begin(x)
#define BEG_END(x) std::begin(x), std::end(x)

using i08 = std::int8_t;
using u08 = std::uint8_t;
using i16 = std::int16_t;
using u16 = std::uint16_t;
using i32 = std::int32_t;
using u32 = std::uint32_t;
using i64 = std::int64_t;
using u64 = std::uint64_t;

extern void insert_uint64(std::vector<std::byte>& bytes, std::uint64_t x) noexcept;
extern void insert_uint32(std::vector<std::byte>& bytes, std::uint32_t x) noexcept;
extern void insert_uint16(std::vector<std::byte>& bytes, std::uint16_t x) noexcept;
extern void insert_uint8(std::vector<std::byte>& bytes, std::uint8_t x) noexcept;
extern std::uint64_t read_uint64(const std::vector<std::byte>& bytes, size_t offset) noexcept;
extern std::uint32_t read_uint32(const std::vector<std::byte>& bytes, size_t offset) noexcept;
extern std::uint16_t read_uint16(const std::vector<std::byte>& bytes, size_t offset) noexcept;
extern std::uint8_t read_uint8(const std::vector<std::byte>& bytes, size_t offset) noexcept;


extern std::uint32_t byte_swap(std::uint32_t x) noexcept;

extern std::filesystem::path get_user_data_path() noexcept;

extern const std::filesystem::path App_Data_Dir_Name;
[[nodiscard]] extern std::filesystem::path get_app_data_path() noexcept;
extern Logs logs;
[[nodiscard]] extern std::string format_errno(errno_t x) noexcept;
