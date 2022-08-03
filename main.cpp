#include <Windows.h>
#include <iostream>
#include <thread>
#include <time.h>
#include "EyeStep/eyestep_utility.h"
#include "EyeStep/eyestep.h"
#include "scheduler.h"
#define getConvention(a) EyeStep::convs[EyeStep::util::getConvention(a)]

void LogFunc(const char* name, int address) {
	if (!EyeStep::util::isPrologue(address)) {
		address = EyeStep::util::getPrologue(address);
	}
	std::cout << name;
	int space = 10 - strlen(name);

	for (int i = 0; i < space; i++)
		std::cout << " ";
	std::cout << ": 0x" << std::hex << EyeStep::util::raslr(address) << " " << getConvention(address) << std::endl;
}

void LogAddr(const char* name, int address) {
	std::cout << name;
	int space = 25 - strlen(name);

	for (int i = 0; i < space; i++)
		std::cout << " ";
	std::cout << ": 0x" << std::hex << EyeStep::util::raslr(address) << std::endl;
}

void LogOff(const char* name, int off) {
	std::cout << name;
	int space = 25 - strlen(name);

	for (int i = 0; i < space; i++)
		std::cout << " ";
	std::cout << ": 0x" << off << std::endl;
}

void LogOffNoHex(const char* name, int off) {
	std::cout << name;
	int space = 25 - strlen(name);

	for (int i = 0; i < space; i++)
		std::cout << " ";
	std::cout << ": " << off << std::endl;
}

void console(const char* title) {
	DWORD old;
	VirtualProtect(reinterpret_cast<PVOID>(&FreeConsole), 1, PAGE_EXECUTE_READWRITE, &old);
	*reinterpret_cast<std::uint8_t*>(&FreeConsole) = 0xC3;
	VirtualProtect(&FreeConsole, 1, old, &old);
	AllocConsole();
	SetConsoleTitleA(title);
	FILE* file_stream;
	freopen_s(&file_stream, "CONOUT$", "w", stdout);
	freopen_s(&file_stream, "CONOUT$", "w", stderr);
	freopen_s(&file_stream, "CONIN$", "r", stdin);
}

void main() {
	EyeStep::open(GetCurrentProcess());

	console("Dumper");

	time_t begin, end;
	time(&begin);
	printf("Scanning...\n");

	auto printxref = EyeStep::scanner::scan_xrefs("Video recording started")[0];
	auto printcalls = EyeStep::util::nextCall(printxref);
	auto printaddy = EyeStep::util::raslr(printcalls);
	LogFunc("Print Address", printcalls);

	auto taskscheduler1 = EyeStep::util::getPointers(EyeStep::util::getPrologue(EyeStep::scanner::scan_xrefs("SchedulerRate")[0]))[3];
	auto taskscheduler1calls = EyeStep::util::nextCall(taskscheduler1);
	auto tasksaddy = EyeStep::util::raslr(taskscheduler1calls);
	LogFunc("taskscheduler1 Address", taskscheduler1calls);

	auto taskscheduler2 = EyeStep::scanner::scan_xrefs("Load ClientAppSettings", true)[0];
	auto taskscheduler2calls = EyeStep::util::prevCall(EyeStep::util::prevCall(EyeStep::util::prevCall(taskscheduler2, true)));
	LogFunc("taskscheduler2 Address", taskscheduler2calls);

	const scheduler_t scheduler{};
	std::cout << "Current scheduler: 0x" << scheduler.get() << "\n";
	scheduler.print_jobs();
	std::cout << "Got Datamodel: 0x" << scheduler.get_datamodel() << "\n";
	std::cout << "Got ScriptContext: 0x" << scheduler.get_script_context() << "\n";
	time(&end);
	std::cout << "Time taken: " << end - begin << " second(s)" << std::endl;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
		std::thread(main).detach();
	return TRUE;
}
