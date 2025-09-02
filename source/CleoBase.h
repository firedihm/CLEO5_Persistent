#pragma once

#include "CDmaFix.h"
#include "CGameMenu.h"
#include "CCodeInjector.h"
#include "CPluginSystem.h"
#include "CGameVersionManager.h"
#include "CScriptEngine.h"
#include "CCustomOpcodeSystem.h"
#include "CModuleSystem.h"
#include "OpcodeInfoDatabase.h"

namespace CLEO
{

enum class InitStage
{
        None,
        Initial,
        OnDraw,
        Done = OnDraw
};

class CCleoInstance
{
    public:
        // order here defines init and deinit order!
        CDmaFix		                        DmaFix;
        CGameMenu		        GameMenu;
        CCodeInjector			CodeInjector;
        CPluginSystem			PluginSystem;
        CGameVersionManager		VersionManager;
        CScriptEngine			ScriptEngine;
        CCustomOpcodeSystem		OpcodeSystem;
        CModuleSystem			ModuleSystem;
        OpcodeInfoDatabase		OpcodeInfoDb;

        CCleoInstance() = default;
        ~CCleoInstance() { Stop(); }

        void Start(InitStage stage);
        void Stop();

        bool IsStarted() const { return m_InitStage != InitStage::None; }
        int GetSaveSlot() const { return m_saveSlot; }

        const std::set<void*>& GetCallbacks(eCallbackId id) { return m_callbacks[id]; }
        void AddCallback(eCallbackId id, void* func) { m_callbacks[id].insert(func); }
        void RemoveCallback(eCallbackId id, void* func) { m_callbacks[id].erase(func); }
        void CallCallbacks(eCallbackId id);
        void CallCallbacks(eCallbackId id, DWORD arg);

    private:
        InitStage m_InitStage = InitStage::None;
        bool m_bGameInProgress = false; // is this really needed?
        int m_saveSlot = -1;
        std::map<eCallbackId, std::set<void*>> m_callbacks;

        void GameBegin();
        void GameEnd();
        void GameRestart();

        // call for InitInstance
        HWND(__cdecl* CreateMainWnd_Orig)(HINSTANCE) = nullptr;
        LRESULT(__stdcall* MainWndProc_Orig)(HWND, UINT, WPARAM, LPARAM) = nullptr;
        static HWND __cdecl OnCreateMainWnd(HINSTANCE hinst);
        static LRESULT __stdcall OnMainWndProc(HWND, UINT, WPARAM, LPARAM);

        // calls to CTheScripts::Init
        void(__cdecl* ScmInit1_Orig)() = nullptr;
        void(__cdecl* ScmInit2_Orig)() = nullptr;
        void(__cdecl* ScmInit3_Orig)() = nullptr;
        static void OnScmInit1();
        static void OnScmInit2();
        static void OnScmInit3();

        // call for Game::Shutdown
        void(__cdecl* GameShutdown_Orig)() = nullptr;
        static void OnGameShutdown();

        // calls for Game::ShutDownForRestart
        void(__cdecl* GameRestart1_Orig)() = nullptr;
        void(__cdecl* GameRestart2_Orig)() = nullptr;
        void(__cdecl* GameRestart3_Orig)() = nullptr;
        static void OnGameRestart1();
        static void OnGameRestart2();
        static void OnGameRestart3();

        // calls to CDebug::DebugDisplayTextBuffer
        void(__cdecl* GameRestartDebugDisplayTextBuffer_Idle_Orig)() = nullptr;
        void(__cdecl* GameRestartDebugDisplayTextBuffer_Frontend_Orig)() = nullptr;
        static void OnDebugDisplayTextBuffer_Idle();
        static void OnDebugDisplayTextBuffer_Frontend();

        void(__cdecl* UpdateGameLogics_Orig)() = nullptr;
        static void __cdecl OnUpdateGameLogics();
};

extern CCleoInstance CleoInstance;

} // namespace CLEO
