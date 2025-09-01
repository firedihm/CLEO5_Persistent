#include "stdafx.h"
#include "CleoBase.h"
#include "Singleton.h"

using namespace CLEO;

CCleoInstance CleoInstance;

void CCleoInstance::Start(InitStage stage)
{
        if (stage > InitStage::Done || stage != GetNextInitStage()) 
                return;

        if (stage == InitStage::Initial) {
                TRACE("CLEO initialization: Phase 1");

                FS::create_directory(Filepath_Cleo);
                FS::create_directory(Filepath_Cleo + "\\cleo_modules");
                FS::create_directory(Filepath_Cleo + "\\cleo_plugins");
                FS::create_directory(Filepath_Cleo + "\\cleo_saves");

                OpcodeInfoDb.Load((Filepath_Cleo + "\\.config\\sa.json").c_str());

                CodeInjector.OpenReadWriteAccess(); // must do this earlier to ensure plugins write access on init
                GameMenu.Inject(CodeInjector);
                DmaFix.Inject(CodeInjector);
                OpcodeSystem.Inject(CodeInjector);
                ScriptEngine.Inject(CodeInjector);

                CodeInjector.ReplaceFunction(OnCreateMainWnd, VersionManager.TranslateMemoryAddress(MA_CALL_CREATE_MAIN_WINDOW), &CreateMainWnd_Orig);

                CodeInjector.ReplaceFunction(OnScmInit1, VersionManager.TranslateMemoryAddress(MA_CALL_INIT_SCM1), &ScmInit1_Orig);
                CodeInjector.ReplaceFunction(OnScmInit2, VersionManager.TranslateMemoryAddress(MA_CALL_INIT_SCM2), &ScmInit2_Orig);
                CodeInjector.ReplaceFunction(OnScmInit3, VersionManager.TranslateMemoryAddress(MA_CALL_INIT_SCM3), &ScmInit3_Orig);

                CodeInjector.ReplaceFunction(OnGameShutdown, VersionManager.TranslateMemoryAddress(MA_CALL_GAME_SHUTDOWN), &GameShutdown_Orig);

                CodeInjector.ReplaceFunction(OnGameRestart1, VersionManager.TranslateMemoryAddress(MA_CALL_GAME_RESTART_1), &GameRestart1_Orig);
                CodeInjector.ReplaceFunction(OnGameRestart2, VersionManager.TranslateMemoryAddress(MA_CALL_GAME_RESTART_2), &GameRestart2_Orig);
                CodeInjector.ReplaceFunction(OnGameRestart3, VersionManager.TranslateMemoryAddress(MA_CALL_GAME_RESTART_3), &GameRestart3_Orig);

                OpcodeSystem.Init();
                PluginSystem.LoadPlugins();
        } else if (stage == InitStage::OnDraw) {
                TRACE("CLEO initialization: Phase 2"); // delayed until menu background drawing

                const_cast<std::string&>(Filepath_User) = GetUserDirectory(); // force update now, as it could be modifed by PortableGTA.asi

                ScriptEngine.InjectLate(CodeInjector);

                CodeInjector.InjectFunction(GetScriptStringParam, gaddrof(::CRunningScript::ReadTextLabelFromScript));
                CodeInjector.ReplaceFunction(OnDebugDisplayTextBuffer_Idle, VersionManager.TranslateMemoryAddress(MA_CALL_DEBUG_DISPLAY_TEXT_BUFFER_IDLE), &GameRestartDebugDisplayTextBuffer_Idle_Orig);
                CodeInjector.ReplaceFunction(OnDebugDisplayTextBuffer_Frontend, VersionManager.TranslateMemoryAddress(MA_CALL_DEBUG_DISPLAY_TEXT_BUFFER_FRONTEND), &GameRestartDebugDisplayTextBuffer_Frontend_Orig);
                CodeInjector.ReplaceFunction(OnUpdateGameLogics, VersionManager.TranslateMemoryAddress(MA_CALL_UPDATE_GAME_LOGICS), &UpdateGameLogics_Orig);

                PluginSystem.LogLoadedPlugins();
        }

        m_InitStage = stage;
}

void CCleoInstance::Stop()
{
        GameEnd();
        PluginSystem.UnloadPlugins();
        m_InitStage = InitStage::None;
}

HWND CCleoInstance::OnCreateMainWnd(HINSTANCE hinst)
{
        CleoSingletonCheck(); // check once for CLEO.asi duplicates

        auto window = CreateMainWnd_Orig(hinst);

        // redirect window handling procedure
        *((size_t*)&MainWndProc_Orig) = GetWindowLongPtr(window, GWLP_WNDPROC);
        SetWindowLongPtr(window, GWLP_WNDPROC, (LONG)OnMainWndProc);

        return window;
}

LRESULT __stdcall CCleoInstance::OnMainWndProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
        switch (msg) {
            case WM_ACTIVATE:
                CallCallbacks(eCallbackId::MainWindowFocus, wparam != 0);
                break;
            case WM_KILLFOCUS:
                CallCallbacks(eCallbackId::MainWindowFocus, false);
                break;
        }

        return MainWndProc_Orig(wnd, msg, wparam, lparam);
}

void CCleoInstance::GameBegin()
{
        if (m_bGameInProgress)
                return;

        m_bGameInProgress = true;
        m_saveSlot = FrontEndMenuManager.m_bWantToLoad ? FrontEndMenuManager.m_nSelectedSaveGame : -1;

        TRACE("Starting new game session, save slot: %d", m_saveSlot);

        Start(CCleoInstance::InitStage::OnDraw); // late initialization if not done yet
        CallCallbacks(eCallbackId::GameBegin, m_saveSlot);
}

void CCleoInstance::GameEnd()
{
        if (!m_bGameInProgress)
                return;

        m_bGameInProgress = false;
        m_saveSlot = -1;

        TRACE("Ending current game session");

        CallCallbacks(eCallbackId::GameEnd);
        ScriptEngine.GameEnd();
        OpcodeSystem.FinalizeScriptObjects();        
}

void CCleoInstance::GameRestart()
{
        if (!m_bGameInProgress)
                return;

        m_bGameInProgress = false;
        m_saveSlot = -1;

        TRACE("Ending current game session");

        CallCallbacks(eCallbackId::GameEnd);
        ScriptEngine.GameRestart();
        OpcodeSystem.FinalizeScriptObjects();        
}

void CCleoInstance::OnScmInit1()
{
        ScmInit1_Orig();
        GameBegin();
}

void CCleoInstance::OnScmInit2() // load save
{
        ScmInit2_Orig();
        GameBegin();
}

void CCleoInstance::OnScmInit3()
{
        ScmInit3_Orig();
        GameBegin();
}

void __declspec(naked) CCleoInstance::OnGameShutdown()
{
        GameEnd();
        static DWORD oriFunc;
        oriFunc = (DWORD)(GameShutdown_Orig);
        _asm jmp oriFunc
}

void __declspec(naked) CCleoInstance::OnGameRestart1()
{
        GameRestart();
        static DWORD oriFunc;
        oriFunc = (DWORD)(GameRestart1_Orig);
        _asm jmp oriFunc
}

void __declspec(naked) CCleoInstance::OnGameRestart2()
{
        GameRestart();
        static DWORD oriFunc;
        oriFunc = (DWORD)(GameRestart2_Orig);
        _asm jmp oriFunc
}

void __declspec(naked) CCleoInstance::OnGameRestart3()
{
        GameRestart();
        static DWORD oriFunc;
        oriFunc = (DWORD)(GameRestart3_Orig);
        _asm jmp oriFunc
}

void __cdecl CCleoInstance::OnDebugDisplayTextBuffer_Idle()
{
        GameRestartDebugDisplayTextBuffer_Idle_Orig();
        CallCallbacks(eCallbackId::DrawingFinished);
}

void __cdecl CCleoInstance::OnDebugDisplayTextBuffer_Frontend()
{
        GameRestartDebugDisplayTextBuffer_Frontend_Orig();
        CallCallbacks(eCallbackId::DrawingFinished);
}

void __cdecl CCleoInstance::OnUpdateGameLogics()
{
        CallCallbacks(eCallbackId::GameProcessBefore);
        UpdateGameLogics_Orig();
        CallCallbacks(eCallbackId::GameProcessAfter);
}

void CCleoInstance::CallCallbacks(eCallbackId id)
{
        for (void* func : m_callbacks[id]) {
            typedef void WINAPI callback(void);
            ((callback*)func)();
        }
}

void CCleoInstance::CallCallbacks(eCallbackId id, DWORD arg)
{
        for (void* func : m_callbacks[id]) {
            typedef void WINAPI callback(DWORD);
            ((callback*)func)(arg);
        }
}

void WINAPI CLEO_ResolvePath(CLEO::CRunningScript* thread, char* inOutPath, DWORD pathMaxLen)
{
        if (thread == nullptr || inOutPath == nullptr || pathMaxLen < 2)
        {
            return; // invalid param
        }

        auto resolved = reinterpret_cast<CCustomScript*>(thread)->ResolvePath(inOutPath);

        if (resolved.length() >= pathMaxLen)
            resolved.resize(pathMaxLen - 1); // and terminator character

        std::memcpy(inOutPath, resolved.c_str(), resolved.length() + 1); // with terminator
}

void WINAPI CLEO_StringListFree(StringList list)
{
        if (list.count > 0 && list.strings != nullptr)
        {
            for (DWORD i = 0; i < list.count; i++)
            {
                free(list.strings[i]);
            }

            free(list.strings);
        }
}

StringList WINAPI CLEO_ListDirectory(CLEO::CRunningScript* thread, const char* searchPath, BOOL listDirs, BOOL listFiles)
{
        if (searchPath == nullptr)
            return {}; // invalid param

        if (!listDirs && !listFiles)
            return {}; // nothing to list, done

        // make absolute
        auto fsSearchPath = FS::path(searchPath);
        if (!fsSearchPath.is_absolute())
        {
            if (thread != nullptr)
                fsSearchPath = ((CCustomScript*)thread)->GetWorkDir() / fsSearchPath;
            else
                fsSearchPath = Filepath_Game / fsSearchPath;
        }

        WIN32_FIND_DATA wfd = { 0 };
        HANDLE hSearch = FindFirstFile(fsSearchPath.string().c_str(), &wfd);
        if (hSearch == INVALID_HANDLE_VALUE)
            return {}; // nothing found

        std::set<std::string> found;
        do
        {
            if (!listDirs && (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) 
                continue; // skip directories

            if (!listFiles && !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                continue; // skip files

            auto path = FS::path(wfd.cFileName);
            if (!path.is_absolute()) // keep absolute in case somebody hooked the APIs to return so
                path = fsSearchPath.parent_path() / path;

            found.insert(path.string());
        } while (FindNextFile(hSearch, &wfd));

        FindClose(hSearch);

        return CreateStringList(found);
}
