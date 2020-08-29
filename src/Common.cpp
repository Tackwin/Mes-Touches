#include "Common.hpp"

#include <Windows.h>
#include <cstring>

#include "file.hpp"
const std::filesystem::path App_Data_Dir_Name{ "Mes Touches" };

[[nodiscard]] std::filesystem::path get_app_data_path() noexcept {
	return get_user_data_path() / App_Data_Dir_Name;
}

[[nodiscard]] std::string get_last_os_error() noexcept {
	LPSTR buffer = nullptr;
	size_t size = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&buffer,
		0,
		NULL
	);

	std::string message(buffer, size);
	LocalFree(buffer);
	return message;
}
[[nodiscard]] std::string format_errno(errno_t x) noexcept {
	std::string result;
	result.resize(100);
	strerror_s(result.data(), result.capacity(), x);
	return result;
}

std::uint32_t byte_swap(std::uint32_t x) noexcept {
	return
		((x >> 24) & 0xff) | // move byte 3 to byte 0
		((x << 8) & 0xff0000) | // move byte 1 to byte 2
		((x >> 8) & 0xff00) | // move byte 2 to byte 1
		((x << 24) & 0xff000000); // byte 0 to byte 3
}


void insert_uint64(std::vector<std::byte>& bytes, std::uint64_t x) noexcept {
	bytes.push_back((std::byte)((x & 0x00000000000000ff) >> 0));
	bytes.push_back((std::byte)((x & 0x000000000000ff00) >> 8));
	bytes.push_back((std::byte)((x & 0x0000000000ff0000) >> 16));
	bytes.push_back((std::byte)((x & 0x00000000ff000000) >> 24));
	bytes.push_back((std::byte)((x & 0x000000ff00000000) >> 32));
	bytes.push_back((std::byte)((x & 0x0000ff0000000000) >> 40));
	bytes.push_back((std::byte)((x & 0x00ff000000000000) >> 48));
	bytes.push_back((std::byte)((x & 0xff00000000000000) >> 56));
};
void insert_uint32(std::vector<std::byte>& bytes, std::uint32_t x) noexcept {
	bytes.push_back((std::byte)((x & 0x000000ff) >> 0));
	bytes.push_back((std::byte)((x & 0x0000ff00) >> 8));
	bytes.push_back((std::byte)((x & 0x00ff0000) >> 16));
	bytes.push_back((std::byte)((x & 0xff000000) >> 24));
};
void insert_uint16(std::vector<std::byte>& bytes, std::uint16_t x) noexcept {
	bytes.push_back((std::byte)((x & 0x00ff) >> 0));
	bytes.push_back((std::byte)((x & 0xff00) >> 8));
};
void insert_uint8(std::vector<std::byte>& bytes, std::uint8_t x) noexcept {
	bytes.push_back((std::byte)x);
};

[[nodiscard]] std::uint32_t read_uint32(const std::vector<std::byte>& bytes, size_t offset) noexcept {
	std::uint32_t x{ 0 };
	x = x | (std::uint32_t)(((std::uint8_t)bytes[offset + 0]) << 0);
	x = x | (std::uint32_t)(((std::uint8_t)bytes[offset + 1]) << 8);
	x = x | (std::uint32_t)(((std::uint8_t)bytes[offset + 2]) << 16);
	x = x | (std::uint32_t)(((std::uint8_t)bytes[offset + 3]) << 24);
	return x;
};
[[nodiscard]] std::uint64_t read_uint64(const std::vector<std::byte>& bytes, size_t offset) noexcept {
	std::uint64_t x{ 0 };
	x = x | (((std::uint64_t)((std::uint8_t)bytes[offset + 0])) << (std::uint64_t)0);
	x = x | (((std::uint64_t)((std::uint8_t)bytes[offset + 1])) << (std::uint64_t)8);
	x = x | (((std::uint64_t)((std::uint8_t)bytes[offset + 2])) << (std::uint64_t)16);
	x = x | (((std::uint64_t)((std::uint8_t)bytes[offset + 3])) << (std::uint64_t)24);
	x = x | (((std::uint64_t)((std::uint8_t)bytes[offset + 4])) << (std::uint64_t)32);
	x = x | (((std::uint64_t)((std::uint8_t)bytes[offset + 5])) << (std::uint64_t)40);
	x = x | (((std::uint64_t)((std::uint8_t)bytes[offset + 6])) << (std::uint64_t)48);
	x = x | (((std::uint64_t)((std::uint8_t)bytes[offset + 7])) << (std::uint64_t)56);
	return x;
};
[[nodiscard]] std::uint16_t read_uint16(const std::vector<std::byte>& raw, size_t offset) noexcept {
	return
		(uint16_t)(((uint8_t)raw[offset + 0]) << 0)
		| (uint16_t)(((uint8_t)raw[offset + 1]) << 8);
};
[[nodiscard]] std::uint8_t read_uint8(const std::vector<std::byte>& bytes, size_t offset) noexcept {
	std::uint8_t x{ 0 };
	x |= (std::uint8_t)(((std::uint8_t)bytes[offset + 0]) << 0);
	return x;
};