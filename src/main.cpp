#include "version.h"

#include "RE/Skyrim.h"
#include "REL/Relocation.h"
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


		RE::UI_MESSAGE_RESULTS Hook_ProcessMessage(RE::UIMessage& a_message)	// 04
		{
			using Message = RE::UI_MESSAGE_TYPE;
			if (a_message.type == Message::kShow) {
				auto ui = RE::UI::GetSingleton();
				auto uiStr = RE::InterfaceStrings::GetSingleton();
				if (ui->IsMenuOpen(uiStr->mapMenu)) {
					*_savedTabIdx = Tab::kQuest;
				} else {
					*_savedTabIdx = Tab::kSystem;
				}
			}

			return _ProcessMessage(this, a_message);
		}


		static void RememberCurrentTabIndex([[maybe_unused]] const RE::FxDelegateArgs& a_params)
		{
			return;
		}


		static void InstallHooks()
		{
			REL::Offset<std::uintptr_t> vTable(RE::Offset::JournalMenu::Vtbl);
			_Accept = vTable.WriteVFunc(0x1, &JournalMenuEx::Hook_Accept);
			_ProcessMessage = vTable.WriteVFunc(0x4, &JournalMenuEx::Hook_ProcessMessage);

			_MESSAGE("Installed hooks");
		}


		using Accept_t = decltype(&RE::JournalMenu::Accept);
		using ProcessMessage_t = decltype(&RE::JournalMenu::ProcessMessage);

		static inline REL::Function<Accept_t> _Accept;
		static inline REL::Function<ProcessMessage_t> _ProcessMessage;
		static inline REL::Offset<Tab*> _savedTabIdx = REL::ID(520167);
	};
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
			_FATALERROR("Loaded in editor, marking as incompatible!");
			return false;
		}

		auto ver = a_skse->RuntimeVersion();
		if (ver <= SKSE::RUNTIME_1_5_39) {
			_FATALERROR("Unsupported runtime version %s!", ver.GetString().c_str());
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
