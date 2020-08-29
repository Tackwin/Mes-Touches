#include "ErrorCode.hpp"

#include <Windows.h>

[[nodiscard]] std::string format_error_code(std::int64_t x) noexcept {
	LPSTR buffer = nullptr;
	size_t size = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		(DWORD)x,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&buffer,
		0,
		NULL
	);

	std::string message(buffer, size);
	LocalFree(buffer);
	return message;
}
