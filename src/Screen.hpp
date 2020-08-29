#pragma once

#include <vector>
#include <cstdint>
#include <optional>

struct Screen {
	std::uint32_t width;
	std::uint32_t height;
	std::uint32_t x;
	std::uint32_t y;
	std::uint32_t unique_hash;


	static constexpr auto Unique_Hash_Size = 32;
	char unique_hash_char[Unique_Hash_Size];

	Screen() = default;
	Screen(Screen&&) = default;
	Screen& operator=(Screen&&) = default;

	Screen(const Screen& s) noexcept {
		width = s.width;
		height = s.height;
		x = s.x;
		y = s.y;
		unique_hash = s.unique_hash;
		memcpy_s(unique_hash_char, Unique_Hash_Size, s.unique_hash_char, Unique_Hash_Size);
	}
	Screen& operator=(Screen& s) noexcept {
		width = s.width;
		height = s.height;
		x = s.x;
		y = s.y;
		unique_hash = s.unique_hash;
		memcpy_s(unique_hash_char, Unique_Hash_Size, s.unique_hash_char, Unique_Hash_Size);
		return *this;
	}

	bool operator==(const Screen& that) const noexcept {
		return memcmp(&unique_hash, &that.unique_hash, Unique_Hash_Size) == 0;
	}
};

namespace std {
	template<>
	struct hash<Screen> {
		size_t operator()(const Screen& s) const noexcept {
			return s.unique_hash;
		}
	};
}

struct Screen_Set {
	std::vector<Screen> screens;

	std::uint32_t virtual_width;
	std::uint32_t virtual_height;
	std::uint32_t main_x;
	std::uint32_t main_y;
};

[[nodiscard]] extern Screen_Set get_all_screens() noexcept;
