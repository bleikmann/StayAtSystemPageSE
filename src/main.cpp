#include "skse64/PluginAPI.h"
#include "skse64_common/skse_version.h"
#include "skse64_common/Relocation.h"
#include "skse64_common/SafeWrite.h"
#include "skse64_common/BranchTrampoline.h"
#include "skse64/GameMenus.h"
#include <shlobj.h>
#include "xbyak/xbyak.h"

void * g_moduleHandle = nullptr;

IDebugLog				gLog;
PluginHandle			g_pluginHandle = kPluginHandle_Invalid;

RelocPtr<uintptr_t> RememberCurrentTabIdxAddr(0x008F48E0);
RelocPtr<uintptr_t> GetCurrentTabIndexAddr(0x008F3ED3);


RelocPtr<uintptr_t> SavedTabIndexAddr(0x02F761C0);
static UInt32 &SavedTabIndex = *(UInt32*)SavedTabIndexAddr.GetUIntPtr();

static UInt32 GetSavedTabIndex(UInt8 rcx, UInt8 rdx)
{
	UInt32 retn = 2;
	if (rcx != rdx)
	{
		MenuManager * mm = MenuManager::GetSingleton();
		if (mm)
		{
			if (mm->IsMenuOpen(&UIStringHolder::GetSingleton()->mapMenu))
			{
				retn = 0;
			}
		}
		else
		{
			retn = rcx;
		}
	}
	else
	{
		retn = rcx;
	}

	return retn;
}

extern "C"
{

	bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info)
	{

		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim Special Edition\\SKSE\\StayAtSystemPage.log");

		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "StayAtSystemPage plugin";
		info->version = 1;

		g_pluginHandle = skse->GetPluginHandle();

		if (skse->isEditor)
		{
			_MESSAGE("loaded in editor, marking as incompatible");
			return false;
		}

		if (skse->runtimeVersion != RUNTIME_VERSION_1_5_50)
		{
			_MESSAGE("This plugin is not compatible with this versin of game.");
			return false;
		}

		if (!g_branchTrampoline.Create(1024 * 64))
		{
			_ERROR("couldn't create branch trampoline. this is fatal. skipping remainder of init process.");
			return false;
		}

		if (!g_localTrampoline.Create(1024 * 64, g_moduleHandle))
		{
			_ERROR("couldn't create codegen buffer. this is fatal. skipping remainder of init process.");
			return false;
		}

		return true;
	}

	bool SKSEPlugin_Load(const SKSEInterface * skse)
	{
		_MESSAGE("Load");
		
		// 0 = quest, 1 = player info, 2 = system
		SavedTabIndex = 2;

		// disable the remembercurrenttabindex callback
		SafeWrite32(RememberCurrentTabIdxAddr.GetUIntPtr(), 0x909090C3);

		// fix for map menu
		struct InstallGetSavedTabIndex_Code : Xbyak::CodeGenerator {
			InstallGetSavedTabIndex_Code(void * buf, uintptr_t funcAddr) : Xbyak::CodeGenerator(4096, buf)
			{
				Xbyak::Label retnLabel;
				Xbyak::Label funcLabel;

				sub(rsp, 0x20);

				mov(dl, byte[rax + 0x20]);
				mov(cl, cl);
				call(ptr[rip + funcLabel]);

				add(rsp, 0x20);

				jmp(ptr[rip + retnLabel]);

				L(funcLabel);
				dq(funcAddr);

				L(retnLabel);
				dq(GetCurrentTabIndexAddr.GetUIntPtr() + 0x5);
			}
		};

		void * codeBuf = g_localTrampoline.StartAlloc();
		InstallGetSavedTabIndex_Code code(codeBuf, GetFnAddr(&GetSavedTabIndex));
		g_localTrampoline.EndAlloc(code.getCurr());

		if (!g_branchTrampoline.Write5Branch(GetCurrentTabIndexAddr.GetUIntPtr(), uintptr_t(code.getCode())))
			return false;
		SafeWrite32(GetCurrentTabIndexAddr.GetUIntPtr() + 5, 0x90909090);

		return true;
	}

}