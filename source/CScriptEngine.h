#pragma once

#include "CCustomScript.h"

namespace CLEO
{

const char cs_ext[] = ".cs";
const char cs4_ext[] = ".cs4";
const char cs3_ext[] = ".cs3";

class CScriptEngine
{
    public:
        BYTE* scmBlock = nullptr;
        BYTE* missionBlock = nullptr;
        int missionIndex = -1;
        static SCRIPT_VAR CleoVariables[0x400]; // also defined in .cpp???

        // main script stuff
        bool NativeScriptsDebugMode = false; // debug mode enabled?
        eCLEO_Version NativeScriptsVersion = eCLEO_Version::CLEO_VER_CUR; // allows using legacy modes
        std::string MainScriptFileDir;
        std::string MainScriptFileName;
        std::string MainScriptCurWorkDir;

        CScriptEngine() = default;
        CScriptEngine(const CScriptEngine&) = delete; // no copying
        ~CScriptEngine() { GameEnd(); }

        void Inject(CCodeInjector&); // Phase 1
        void InjectLate(CCodeInjector&); // Phase 2

        void GameBegin(); // call after new game started
        void GameEnd();
        void GameRestart();

        friend class CCustomScript;
        std::list<CCustomScript*> CustomScripts;
        std::list<CCustomScript*> ScriptsWaitingForDelete;
        std::set<unsigned long> InactiveScriptHashes;
        CCustomScript* CustomMission = nullptr;
        CCustomScript* LastScriptCreated = nullptr;

        CCustomScript* LoadScript(const char* filePath);
        CCustomScript* CreateCustomScript(CRunningScript* fromThread, const char* filePath, int label);



        static void __cdecl OnDrawScriptText(char beforeFade);
        static void __fastcall OnProcessScript(CLEO::CRunningScript*);

        CRunningScript* FindScriptNamed(const char* threadName, bool standardScripts, bool customScripts, size_t resultIndex = 0); // can be called multiple times to find more scripts named threadName. resultIndex should be incremented until the method returns nullptr
        CRunningScript* FindScriptByFilename(const char* path, size_t resultIndex = 0); // if path is not absolute it will be resolved with cleo directory as root
        bool IsActiveScriptPtr(const CRunningScript*) const; // leads to active script? (regular or custom)
        bool IsValidScriptPtr(const CRunningScript*) const; // leads to any script? (regular or custom)


        inline CCustomScript* GetCustomMission() { return CustomMission; }
        inline size_t WorkingScriptsCount() { return CustomScripts.size(); }

        static SCRIPT_VAR* GetScriptParamPointer(CRunningScript* thread);

        // params into/from opcodeParams array
        static void GetScriptParams(CRunningScript* script, BYTE count);
        static void SetScriptParams(CRunningScript* script, BYTE count);

        static void DrawScriptText_Orig(char beforeFade);

    private:
        void(__fastcall* ProcessScript_Orig)(CLEO::CRunningScript*) = nullptr;
        void(__cdecl* DrawScriptTextAfterFade_Orig)(char beforeFade) = nullptr;
        void(__cdecl* DrawScriptTextBeforeFade_Orig)(char beforeFade) = nullptr;

        bool m_bGameInProgress = false;
        bool m_bReregisterPersistentScripts = false;

        
        void LoadMainScriptStuff();

        // CLEO saves
        void LoadState(int saveSlot);
        void SaveState();

        void AddCustomScript(CCustomScript*);
        void RemoveScript(CRunningScript*); // native or custom
        void RemoveCustomScript(CCustomScript*);

        void LoadAllCustomScripts();
        void RemoveAllCustomScripts();

        // remove/re-add to active scripts queue
        void UnregisterAllCustomScripts();
        void ReregisterAllCustomScripts();
        void ReregisterPersistentScripts();
};

// reimplemented hook of original game's procedure
// returns buff or pointer provided by script, nullptr on fail
// WARNING: Null terminator ommited if not enought space in the buffer!
const char* __fastcall GetScriptStringParam(CRunningScript* thread, int dummy, char* buff, int buffLen); 

} // namespace CLEO
