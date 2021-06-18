#include "Ease.hpp"

EASE_WATCH_ME

Build build_cbt_win64_hook(Flags flags) noexcept {
	auto b = Build::get_default(flags);
	b.target = Build::Target::Shared;
	b.name = "win64_hook";

	b.add_define("ARCH_64");
	b.add_source("src/cbt_hook.cpp");

	b.add_library("User32");

	return b;
}

Build build_cbt_win32_hook(Flags flags) noexcept {
	auto b = Build::get_default(flags);
	b.target = Build::Target::Shared;
	b.arch = Build::Arch::x86;
	b.name = "win32_hook";

	b.add_define("ARCH_32");
	b.add_source("src/cbt_hook.cpp");

	b.add_library("User32");

	return b;
}

Build build_32_bit_process(Flags flags) noexcept {
	auto b = Build::get_default(flags);

	b.arch = Build::Arch::x86;
	b.name = "Mes_Touches_32";
	b.add_source("src/Main32.cpp");
	b.add_library("win32_hook");

	return b;
}

Build build(Flags flags) noexcept {
	auto win32_hook = build_cbt_win32_hook(flags);
	auto win64_hook = build_cbt_win64_hook(flags);
	auto proc32     = build_32_bit_process(flags);
	auto proc64     = Build::get_default  (flags);

	proc64.name = "Mes_Touches";

	proc64.add_header("./src/");
	proc64.add_header("./src/imgui/");
	proc64.add_source_recursively("./src/");
	proc64.del_source("src/cbt_hook.cpp");
	proc64.del_source("src/Main32.cpp");
	proc64.del_source_recursively("./src/OS/");
	if (Env::Win32) proc64.add_source_recursively("./src/OS/win/");
	//if (Env::Win32) b.add_source("./src/Mes_Touches.rc");

	proc64.add_define("GLEW_STATIC");
	proc64.add_define("IMGUI_IMPL_OPENGL_LOADER_GLEW");

	proc64.add_library("version");
	proc64.add_library("kernel32");
	proc64.add_default_win32();
	proc64.add_library("opengl32");
	proc64.add_library("gdi32");
	proc64.add_library("Advapi32");
	proc64.add_library("lib/glew32");

	proc64.add_library("win64_hook");

	return Build::sequentials({win32_hook, win64_hook, proc64, proc32});
}