#include "Screen.hpp"

#include <Windows.h>

Screen_Set get_all_screens() noexcept {
	struct MonitorRects {
		std::vector<HMONITOR> handles;
		RECT rcCombined;
		char unique_id[128];

		static BOOL CALLBACK MonitorEnum(
			HMONITOR hMon, HDC, LPRECT lprcMonitor, LPARAM pData
		) noexcept {
			MonitorRects* pThis = reinterpret_cast<MonitorRects*>(pData);
			pThis->handles.push_back(hMon);
			UnionRect(&pThis->rcCombined, &pThis->rcCombined, lprcMonitor);
			return TRUE;
		}

		MonitorRects() noexcept {
			SetRectEmpty(&rcCombined);
			EnumDisplayMonitors(0, 0, MonitorEnum, (LPARAM)this);
		}
	};

	MonitorRects monitors;

	Screen_Set set;
	set.main_x = -monitors.rcCombined.left;
	set.main_y = -monitors.rcCombined.top;
	set.virtual_width = monitors.rcCombined.right - monitors.rcCombined.left;
	set.virtual_height = monitors.rcCombined.bottom* - monitors.rcCombined.top;

	for (size_t i = 0; i < monitors.handles.size(); ++i) {
		MONITORINFOEX info;
		info.cbSize = sizeof(MONITORINFOEX);
		GetMonitorInfo(monitors.handles[i], &info);

		Screen s;
		s.x = info.rcMonitor.left + set.main_x;
		s.y = info.rcMonitor.top + set.main_y;
		s.width = info.rcMonitor.right - info.rcMonitor.left;
		s.height = info.rcMonitor.bottom - info.rcMonitor.top;
		memset(s.unique_hash_char, 0, Screen::Unique_Hash_Size);
		// We use strcpy and not memcpy because there info.szDevice is null terminated and after
		// 0 there is a bunch of garbage... But _we_ want to zero initialize everything else.
		
		// and we use custom strcpy because for some dumb reason strcpy modify the dest buffer
		// past the end of the null char of src...
		for (uint32_t j = 0; j < strnlen_s(info.szDevice, 32); ++j) {
			s.unique_hash_char[j] = info.szDevice[j];
		}

		if (info.dwFlags & MONITORINFOF_PRIMARY) {
			set.screens.push_back(s);
			auto temp = set.screens.front();
			set.screens.front() = set.screens.back();
			set.screens.back() = temp;
		}
		else {
			set.screens.push_back(s);
		}
	}

	return set;
}


