#include "skse64_common/BranchTrampoline.h"
#include "skse64_common/SafeWrite.h"
#include "skse64_common/skse_version.h"

#include "version.h"

#include "RE/Skyrim.h"
#include "SKSE/API.h"


namespace
{
	using Accept_t = function_type_t<decltype(&RE::IMenu::Accept)>;
	Accept_t* _Accept = 0;


	void RememberCurrentTabIndex(const RE::FxDelegateArgs& a_params)
	{
		return;
	}


	void Hook_Accept(RE::IMenu* a_journalMenu, RE::FxDelegateHandler::CallbackProcessor* a_cbReg)
	{
		_Accept(a_journalMenu, a_cbReg);
		a_journalMenu->fxDelegate->callbacks.Remove("RememberCurrentTabIndex");
		a_cbReg->Process("RememberCurrentTabIndex", RememberCurrentTabIndex);
	}


	void InstallHooks()
	{
		enum
		{
			kQuest,
			kPlayerInfo,
			kSystem
		};

		// 83 79 30 00 76 10
		REL::Offset<UInt32*> savedTabIdx(0x02F4F1C0);	// 1_5_97
		*savedTabIdx = kSystem;

		REL::Offset<Accept_t**> vFunc(RE::Offset::JournalMenu::Vtbl + (0x8 * 0x1));
		_Accept = *vFunc;
		SafeWrite64(vFunc.GetAddress(), unrestricted_cast<std::uintptr_t>(&Hook_Accept));

		_MESSAGE("Installed hooks");
	}
}


extern "C"
{
	bool SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
	{
		SKSE::Logger::OpenRelative(FOLDERID_Documents, L"\\My Games\\Skyrim Special Edition\\SKSE\\StayAtSystemPage.log");
		SKSE::Logger::SetPrintLevel(SKSE::Logger::Level::kDebugMessage);
		SKSE::Logger::SetFlushLevel(SKSE::Logger::Level::kDebugMessage);
		SKSE::Logger::UseLogStamp(true);

		_MESSAGE("StayAtSystemPage v%s", SYSP_VERSION_VERSTRING);

		a_info->infoVersion = SKSE::PluginInfo::kVersion;
		a_info->name = "StayAtSystemPage";
		a_info->version = SYSP_VERSION_MAJOR;

		if (a_skse->IsEditor()) {
			_FATALERROR("Loaded in editor, marking as incompatible!\n");
			return false;
		}

		switch (a_skse->RuntimeVersion()) {
		case RUNTIME_VERSION_1_5_97:
			break;
		default:
			_FATALERROR("Unsupported runtime version %s!\n", a_skse->UnmangledRuntimeVersion().c_str());
			return false;
		}

		return true;
	}


	bool SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
	{
		_MESSAGE("StayAtSystemPage loaded");

		if (!SKSE::Init(a_skse)) {
			return false;
		}

		InstallHooks();

		return true;
	}
}
