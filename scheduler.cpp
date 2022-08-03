#include "scheduler.h"
#include "EyeStep/eyestep_utility.h"
#include <iostream>

namespace func_defs
{
	using rbx_getscheduler_t = std::uintptr_t(__cdecl*)();
	using rbx_output_t = void(__fastcall*)(std::int16_t output_type, const char* str);
	using rbx_getstate_t = std::uintptr_t(__thiscall*)(std::uintptr_t SC, int* state_type);
	using rbx_pushvfstring_t = int(__cdecl*)(std::uintptr_t rl, const char* fmt, ...);
	using rbx_psuedo2adr_t = std::uintptr_t* (__fastcall*)(std::uintptr_t rl, int idx);
}	


namespace offsets
{
	namespace scheduler
	{
		constexpr std::uintptr_t jobs_start = 0x134;
		constexpr std::uintptr_t jobs_end = 0x138;
		constexpr std::uintptr_t fps = 0x118;
	}

	namespace job
	{
		constexpr std::uintptr_t name = 0x10;
	}

	namespace waiting_scripts_job
	{
		constexpr std::uintptr_t datamodel = 0x28;
		constexpr std::uintptr_t script_context = 0x130;
	}

	namespace identity
	{
		constexpr std::uintptr_t extra_space = 0x48;
		constexpr std::uintptr_t identity = 0x18;
	}

	namespace luastate
	{
		constexpr std::uintptr_t top = 0x14;
		constexpr std::uintptr_t base = 0x8;
	}

	namespace luafunc
	{
		constexpr std::uintptr_t func = 16;
	}
}


scheduler_t::scheduler_t()
{
	auto taskscheduler1 = EyeStep::util::getPointers(EyeStep::util::getPrologue(EyeStep::scanner::scan_xrefs("SchedulerRate")[0]))[3];
	auto taskscheduler1calls = EyeStep::util::nextCall(taskscheduler1);
	auto tasksaddy = EyeStep::util::raslr(taskscheduler1calls);
	//const auto rbx_getscheduler = reinterpret_cast<func_defs::rbx_getscheduler_t>(reinterpret_cast<std::uintptr_t>(GetModuleHandle(nullptr)) + EyeStep::util::aslr((int)tasksaddy));
	func_defs::rbx_getscheduler_t rbx_getscheduler = reinterpret_cast<func_defs::rbx_getscheduler_t>(EyeStep::util::aslr( EyeStep::util::raslr(taskscheduler1calls)));
	this->taskscheduler = rbx_getscheduler();

	std::uintptr_t waiting_scripts_job = this->get_waiting_scripts_job();
	this->datamodel = *reinterpret_cast<std::uintptr_t*>(waiting_scripts_job + offsets::waiting_scripts_job::datamodel);
	this->script_context = *reinterpret_cast<std::uintptr_t*>(waiting_scripts_job + offsets::waiting_scripts_job::script_context);
}

std::uintptr_t scheduler_t::get() const
{
	return this->taskscheduler;
}

std::uintptr_t scheduler_t::get_datamodel() const
{
	return this->datamodel;
}

std::uintptr_t scheduler_t::get_script_context() const
{
	return this->script_context;
}

//std::uintptr_t scheduler_t::get_global_luastate() const
//{
//	int state_type = 0;
//	return rbx_getstate(this->get_script_context(), &state_type);
//}
//

void scheduler_t::print_jobs() const
{
	for (std::uintptr_t& job : this->get_jobs())
	{
		std::string* job_name = reinterpret_cast<std::string*>(job + offsets::job::name);
		std::cout << "Found job: " << job_name->c_str() << "\n";
	}

	MessageBoxA(NULL, "Thank you for using headhunter, created by fishy!", "Thanks!", NULL);
}


std::uintptr_t scheduler_t::get_waiting_scripts_job() const
{
	std::uintptr_t last_job; // sometimes 2 jobs, sometimes 1, idfk what determines but the last job is ((USUALLY)) the right one? idk i cba to rev man
	for (std::uintptr_t& job : this->get_jobs())
	{
		if (std::string* job_name = reinterpret_cast<std::string*>(job + offsets::job::name); *job_name == "WaitingHybridScriptsJob")
		{
			std::printf("potential: 0x%08X\n", *reinterpret_cast<std::uintptr_t*>(job + offsets::waiting_scripts_job::datamodel));
			last_job = job;
		}
	}

	return last_job;
}


std::vector<std::uintptr_t> scheduler_t::get_jobs() const
{
	std::vector<std::uintptr_t> jobs;
	std::uintptr_t* current_job = *reinterpret_cast<std::uintptr_t**>(this->taskscheduler + offsets::scheduler::jobs_start);
	do {
		jobs.push_back(*current_job);
		current_job += 2;
	} while (current_job != *reinterpret_cast<std::uintptr_t**>(this->taskscheduler + offsets::scheduler::jobs_end));

	return jobs;
}

void scheduler_t::hook_waiting_scripts_job(void* hook, std::uintptr_t& original_func) const
{
	std::cout << "Hooking WaitingScriptsJob!\n";

	std::uintptr_t waiting_scripts_job = this->get_waiting_scripts_job();

	void** vtable = new void* [6]; // make new vtable
	memcpy(vtable, *reinterpret_cast<void**>(waiting_scripts_job), 0x18); // clone contents
	original_func = reinterpret_cast<std::uintptr_t>(vtable[2]); // grab func needed

	vtable[2] = hook; // override func
	*reinterpret_cast<void***>(waiting_scripts_job) = vtable; // replace with new vtable

	std:: cout << "Hooked!\n";
}
