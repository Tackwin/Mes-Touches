#include "OS/FileInfo.hpp"
#include "Windows.h"
#include "Common.hpp"

std::optional<std::string> get_exe_description(
	const std::filesystem::path& path
) noexcept {
	auto filename = path.native().c_str();

	int ver_info_size = GetFileVersionInfoSizeW(filename, NULL);
	if (!ver_info_size) return std::nullopt;

	auto ver_info = new BYTE[ver_info_size];
	defer { delete [] ver_info; };
	if (!GetFileVersionInfoW(filename, NULL, ver_info_size, ver_info))
		return std::nullopt;

	struct LANGANDCODEPAGE {
		WORD wLanguage;
		WORD wCodePage;
	} *trans_arr;

	UINT trans_arr_len = 0;
	if (!VerQueryValueW(
		ver_info,
		L"\\VarFileInfo\\Translation",
		(LPVOID*)&trans_arr,
		&trans_arr_len
	)) {
		return std::nullopt;
	}

	trans_arr_len /= sizeof(LANGANDCODEPAGE);
	if (trans_arr_len == 0) return std::nullopt;

	wchar_t file_desc_key[256];
	wchar_t* file_desc = NULL;
	for (size_t i = 0; i < trans_arr_len; ++i) {
		wsprintfW(
			file_desc_key,
			L"\\StringFileInfo\\%04x%04x\\FileDescription",
			trans_arr[i].wLanguage,
			trans_arr[i].wCodePage
		);

		UINT file_decs_size;

		auto ver_value = VerQueryValueW(
			ver_info, file_desc_key, (LPVOID*)&file_desc, &file_decs_size
		);
		auto is_system_lang =
			GetSystemDefaultUILanguage() == trans_arr[i].wLanguage;
		if (ver_value && is_system_lang) break;
	}
	if (file_desc == NULL) return std::nullopt;

	auto size = WideCharToMultiByte(
		CP_UTF8, 0, file_desc, -1, nullptr, 0, NULL, NULL
	);
	if (size == 1) return std::nullopt; // If after that we don't know the name then there is no point to continue.

	std::string str;
	str.resize(size);
	WideCharToMultiByte(
		CP_UTF8,
		0,
		file_desc,
		-1,
		str.data(),
		str.size(),
		NULL,
		NULL
	);

	return str;
}