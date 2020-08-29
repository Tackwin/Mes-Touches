#include "TimeInfo.hpp"

//define all function implementable by standard
#include <chrono>

[[nodiscard]] uint64_t get_microseconds_epoch() noexcept {
	using namespace std::chrono;
	return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
}
[[nodiscard]] uint64_t get_milliseconds_epoch() noexcept {
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}
[[nodiscard]] uint64_t get_seconds_epoch() noexcept {
	using namespace std::chrono;
	return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

