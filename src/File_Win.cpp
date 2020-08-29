#include "file.hpp"
#include <assert.h>
#include <ShlObj.h>
#include <stdio.h>

#include "Common.hpp"
#include "Logs.hpp"

std::filesystem::path get_user_data_path() noexcept {
	PWSTR buffer;
	auto result = SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, NULL, &buffer);

	if (result != S_OK) {
		ErrorDescription error;
		error.location = "File_Win.cpp get_user_data_path";
		error.quick_desc = "Couldn't retrieve the path of ProgramData.";
		error.message =
			"SHGetKnownFolderPath call failed with arguments: "
			"FOLDERID_LocalAppData, KF_FLAG_CREATE, NULL, &buffer.\n"
			"We default to return the current working directory of the application."
			"The error code was";
		switch (result)
		{
		case E_FAIL:
			error.message += "E_FAIL";
			error.message +=
				" Among other things, this value can indicate that the rfid parameter "
				"references a KNOWNFOLDERID which does not have a path"
				"(such as a folder marked as KF_CATEGORY_VIRTUAL).";
		case E_INVALIDARG:
			error.message += "E_INVALIDARG";
			error.message +=
				" Among other things, this value can indicate that the rfid parameter references "
				"a KNOWNFOLDERID have a path that is not present on the system."
				"Not all KNOWNFOLDERID values are present on all systems."
				"Use IKnownFolderManager::GetFolderIds to retrieve the set of KNOWNFOLDERID"
				"values for the current system.";
		default:
			error.message += " unknown.";
			break;
		}
		error.type = ErrorDescription::Type::FileIO;
		logs.lock_and_write(error);
		return std::filesystem::current_path();
	}
	defer{ CoTaskMemFree(buffer); };

	auto str = std::wstring{ buffer };
	std::filesystem::path p{ str };

	return p;
}


std::optional<std::vector<std::byte>> file_read_byte(const std::filesystem::path& path) noexcept {
	if (!std::filesystem::is_regular_file(path)) {
		logs.lock_and_write(
			"std::filesystem::is_regular_file: "
			+ path.generic_string()+ " " + std::filesystem::current_path().generic_string()
		);
		return std::nullopt;
	}

	FILE* file;
	auto err = fopen_s(&file, path.generic_string().c_str(), "rb");
	if (!file || err) {
		logs.lock_and_write("file_read_byte, fopen: " + std::to_string(err));
		return std::nullopt;
	}
	defer{ fclose(file); };

	fseek(file, 0, SEEK_END);
	size_t length = (size_t)ftell(file);
	rewind(file);

	std::vector<std::byte> bytes(length);
	size_t read = fread(bytes.data(), sizeof(std::byte), length, file);
	if (read != length) {
		logs.lock_and_write("file_read_byte, fread: " + std::to_string(err));
		return std::nullopt;
	}

	return std::move(bytes);
}

int
file_write_byte(const std::vector<std::byte>& bytes, const std::filesystem::path& path) noexcept {
	FILE* f;
	auto err = fopen_s(&f, path.generic_string().c_str(), "rb+");
	if (!f || err) return err;
	defer{ fclose(f); };

	auto wrote = fwrite(bytes.data(), 1, bytes.size(), f);
	if (wrote != bytes.size()) return EIO;

	return 0;
}


int file_overwrite_byte(
	const std::vector<std::byte>& bytes, const std::filesystem::path& path
) noexcept {
	FILE* f;
	auto err = fopen_s(&f, path.generic_string().c_str(), "wb+");
	if (!f || err) {
		printf("Error: %d\n", err);
		return err;
	}
	defer{ fclose(f); };

	auto wrote = fwrite(bytes.data(), 1, bytes.size(), f);
	if (wrote != bytes.size()) return EIO;

	return 0;
}


std::optional<uint32_t> file_read_uint32_t(const std::filesystem::path& path, size_t offset) noexcept {
	FILE* f;
	auto err = fopen_s(&f, path.generic_string().c_str(), "rb");
	if (!f || err) {
		logs.lock_and_write("file_read_uint32_t, fopen: " + std::to_string(err));
		return std::nullopt;
	}
	defer{ fclose(f); };

	fseek(f, 0, SEEK_END);
	size_t f_length = ftell(f);
	rewind(f);

	if (f_length <= offset) {
		logs.lock_and_write(
			"file_read_uint32_t, fseek: " + std::to_string(err) + ": " + std::to_string(offset)
		);
		return std::nullopt;
	}

	fseek(f, (long)offset, SEEK_CUR);
	uint8_t x[4];
	size_t read = fread(x, 1, 4, f);
	if (read != 4) {
		logs.lock_and_write("file_read_uint32_t, fopen: " + std::to_string(err));
		return std::nullopt;
	}

	return	(uint32_t)(((uint8_t)x[0]) << 0)
		|	(uint32_t)(((uint8_t)x[1]) << 8)
		|	(uint32_t)(((uint8_t)x[2]) << 16)
		|	(uint32_t)(((uint8_t)x[3]) << 24);
}

int file_write_integer(const std::filesystem::path& path, uint32_t x, size_t offset) noexcept {
	FILE* f;
	auto err = fopen_s(&f, path.generic_string().c_str(), "rb+");
	if (!f || err) return err;
	defer{ fclose(f); };

	fseek(f, offset, SEEK_CUR);
	uint8_t bytes[4] = {
		(uint8_t)(x << 0 ),
		(uint8_t)(x << 8 ),
		(uint8_t)(x << 16),
		(uint8_t)(x << 24)
	};

	fseek(f, 0, SEEK_END);
	size_t f_length = ftell(f);
	rewind(f);

	if (f_length <= offset) return EIO;
	fseek(f, (long)offset, SEEK_CUR);

	auto wrote = fwrite(bytes, 1, 4, f);
	if (wrote != 4) return EIO;

	return 0;
}

int file_insert_byte(
	const std::filesystem::path& path, const std::vector<std::byte>& x, size_t offset
) noexcept {
	FILE* f;
	auto err = fopen_s(&f, path.generic_string().c_str(), "rb+");
	if (!f || err) return err;
	defer{ fclose(f); };

	fseek(f, 0, SEEK_END);
	size_t f_length = ftell(f);
	rewind(f);

	if (f_length < offset) return EIO;
	fseek(f, (long)offset, SEEK_CUR);

	std::vector<std::byte> temp;
	temp.resize(f_length - offset);

	size_t read = fread(temp.data(), sizeof(std::byte), temp.size(), f);
	if (read != temp.size()) return EIO;

	rewind(f);
	fseek(f, offset, SEEK_CUR);

	auto wrote = fwrite(x.data(), 1, x.size(), f);
	if (wrote != x.size()) {
		// we try to restore the file.
		rewind(f);
		fseek(f, offset, SEEK_CUR);

		fwrite(temp.data(), 1, temp.size(), f);
		return EIO;
	}

	rewind(f);
	fseek(f, offset + wrote, SEEK_CUR);

	wrote = fwrite(temp.data(), 1, temp.size(), f);
	if (wrote != temp.size()) return EIO;

	return 0;
}

int file_replace_byte(
	const std::filesystem::path& path, const std::vector<std::byte>& x, size_t offset
) noexcept {
	FILE* f;
	auto err = fopen_s(&f, path.generic_string().c_str(), "rb+");
	if (!f || err) return err;
	defer{ fclose(f); };

	fseek(f, 0, SEEK_END);
	size_t f_length = ftell(f);
	rewind(f);

	if (f_length <= offset) return EIO;
	fseek(f, offset, SEEK_CUR);

	auto wrote = fwrite(x.data(), 1, x.size(), f);
	if (wrote != x.size()) return EIO;

	return 0;
}

std::optional<uint64_t> get_file_length(const std::filesystem::path& path) noexcept{
	FILE* f;
	auto err = fopen_s(&f, path.generic_string().c_str(), "rb+");
	if (!f || err) {
		logs.lock_and_write("file_read_uint32_t, fopen: " + std::to_string(err));
		return std::nullopt;
	}
	defer{ fclose(f); };

	err = fseek(f, 0, SEEK_END);
	if (!err) {
		logs.lock_and_write("file_read_uint32_t, fseek: " + std::to_string(err));
		return std::nullopt;
	}

	return (uint64_t)ftell(f);
}


int file_replace_uint32_t(const std::filesystem::path& path, uint32_t x, size_t offset) noexcept {
	std::vector<std::byte> bytes;
	bytes.push_back((std::byte)((x & 0x000000ff) >> 0));
	bytes.push_back((std::byte)((x & 0x0000ff00) >> 8));
	bytes.push_back((std::byte)((x & 0x00ff0000) >> 16));
	bytes.push_back((std::byte)((x & 0xff000000) >> 24));

	return file_replace_byte(path, bytes, offset);
}
