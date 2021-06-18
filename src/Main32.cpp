__declspec(dllimport) extern bool install_hook_32();
__declspec(dllimport) extern bool uninstall_hook_32();

int main(int argc, char** argv) {

	if (argc > 2 && argv[1][0] == 'A')   install_hook_32();
	if (argc > 2 && argv[1][0] == 'D') uninstall_hook_32();

	return 0;
}