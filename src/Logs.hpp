#pragma once
#include <mutex>
#include <vector>
#include <string>

struct ErrorDescription {
	std::string quick_desc;
	std::string message;
	std::string location;

	enum class Type {
		FileIO,
		Hook,
		Notify,
		Reg,
		Count
	} type;
};

struct LogEntry {
	std::vector<std::string> tag;
	std::string message;
};

struct Logs {
	std::vector<ErrorDescription> errors;
	std::vector<LogEntry> entries;

	std::vector<std::string> list;
	std::mutex mutex;
	bool show{ false };


	void lock_and_clear() noexcept;
	void lock_and_write(const std::string& str) noexcept;
	void lock_and_write(LogEntry entry) noexcept;
	void lock_and_write(ErrorDescription error) noexcept;
};

struct LogWindow {
	bool open{ false };

	void render(Logs& logs) noexcept;
};


