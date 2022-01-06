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

	void InitializeLog()
	{
#ifndef NDEBUG
		auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
		auto path = logger::log_directory();
		if (!path) {
			util::report_and_fail("Failed to find standard logging directory"sv);
		}

		*path /= fmt::format("{}.log"sv, Plugin::NAME);
		auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

#ifndef NDEBUG
		const auto level = spdlog::level::trace;
#else
		const auto level = spdlog::level::info;
#endif

		auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));
		log->set_level(level);
		log->flush_on(level);

		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);
	}
}


extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;

	v.PluginVersion(Plugin::VERSION);
	v.PluginName(Plugin::NAME);

	v.UsesAddressLibrary(true);
	v.CompatibleVersions({ SKSE::RUNTIME_LATEST });

	return v;
}();

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();
	logger::info("{} v{}"sv, Plugin::NAME, Plugin::VERSION.string());

	SKSE::Init(a_skse);

	return true;
}
