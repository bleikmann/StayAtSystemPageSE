#include "skse64_common/SafeWrite.h"
#include "skse64_common/skse_version.h"

#include "version.h"

#include "RE/Skyrim.h"
#include "SKSE/API.h"


namespace
{
	class JournalMenuEx : public RE::JournalMenu
	{
	public:
		enum class Tab : UInt32
		{
			kQuest,
			kPlayerInfo,
			kSystem
		};


		void Hook_Accept(RE::FxDelegateHandler::CallbackProcessor* a_cbReg)	// 01
		{
			_Accept(this, a_cbReg);
			fxDelegate->callbacks.Remove("RememberCurrentTabIndex");
			a_cbReg->Process("RememberCurrentTabIndex", RememberCurrentTabIndex);
		}


		RE::IMenu::Result Hook_ProcessMessage(RE::UIMessage* a_message)	// 04
		{
			using Message = RE::UIMessage::Message;
			if (a_message && a_message->message == Message::kOpen) {
				auto mm = RE::MenuManager::GetSingleton();
				auto uiStr = RE::InterfaceStrings::GetSingleton();
				if (mm->IsMenuOpen(uiStr->mapMenu)) {
					*_savedTabIdx = Tab::kQuest;
				} else {
					*_savedTabIdx = Tab::kSystem;
				}
			}

			return _ProcessMessage(this, a_message);
		}


		static void RememberCurrentTabIndex(const RE::FxDelegateArgs& a_params)
		{
			return;
		}


		static void InstallHooks()
		{
			{
				REL::Offset<Accept_t**> vFunc(RE::Offset::JournalMenu::Vtbl + (0x8 * 0x1));
				_Accept = *vFunc;
				SafeWrite64(vFunc.GetAddress(), unrestricted_cast<std::uintptr_t>(&JournalMenuEx::Hook_Accept));
			}

			{
				REL::Offset<ProcessMessage_t**> vFunc(RE::Offset::JournalMenu::Vtbl + (0x8 * 0x4));
				_ProcessMessage = *vFunc;
				SafeWrite64(vFunc.GetAddress(), unrestricted_cast<std::uintptr_t>(&JournalMenuEx::Hook_ProcessMessage));
			}

			_MESSAGE("Installed hooks");
		}


		using Accept_t = function_type_t<decltype(&RE::JournalMenu::Accept)>;
		using ProcessMessage_t = function_type_t<decltype(&RE::JournalMenu::ProcessMessage)>;

		static inline Accept_t* _Accept = 0;
		static inline ProcessMessage_t* _ProcessMessage = 0;
		static REL::Offset<Tab*> _savedTabIdx;
	};


	// 83 79 30 00 76 10
	decltype(JournalMenuEx::_savedTabIdx) JournalMenuEx::_savedTabIdx(0x02F4F1C0);	// 1_5_97
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

		JournalMenuEx::InstallHooks();

		return true;
	}
}
