#include "stdafx.h"
#include "CGameMenu.h"
#include "CleoBase.h"
#include "CDebug.h"

namespace CLEO
{
    void CGameMenu::Inject(CCodeInjector& inj)
    {
        TRACE("Injecting MenuStatusNotifier...");
        inj.ReplaceFunction(HOOK_DrawMenuBackground, CleoInstance.VersionManager.TranslateMemoryAddress(MA_CALL_CTEXTURE_DRAW_BG_RECT), &DrawMenuBackground_Orig);
    }

    void __fastcall CGameMenu::HOOK_DrawMenuBackground(CSprite2d* texture, int dummy, CRect* rect, RwRGBA *color)
    {
        CleoInstance.Start(CleoInstance::InitStage::OnDraw); // late initialization

        CleoInstance.GameMenu.DrawMenuBackground_Orig(texture, dummy, rect, color);

        CFont::SetBackground(false, false);
        CFont::SetWrapx(640.0f);
        CFont::SetFontStyle(FONT_MENU);
        CFont::SetProportional(true);
        CFont::SetOrientation(ALIGN_LEFT);

        CFont::SetColor({ 0xAD, 0xCE, 0xC4, 0xFF });
        CFont::SetEdge(1);
        CFont::SetDropColor({ 0x00, 0x00, 0x00, 0xFF });

        const float fontSize = 0.5f / 448.0f;
        const float aspect = (float)RsGlobal.maximumWidth / RsGlobal.maximumHeight;
        const float subtextHeight = 0.75f; // factor of first line height
        float sizeX = fontSize * 0.5f / aspect * RsGlobal.maximumWidth;
        float sizeY = fontSize * RsGlobal.maximumHeight;

        float posX = 25.0f * sizeX; // left margin
        float posY = RsGlobal.maximumHeight - 15.0f * sizeY; // bottom margin

        auto cs_count = CleoInstance.ScriptEngine.WorkingScriptsCount();
        auto plugin_count = CleoInstance.PluginSystem.GetNumPlugins();
        if (cs_count || plugin_count)
        {
            posY -= 15.0f * sizeY; // add space for bottom text
        }

        // draw CLEO version text
        std::ostringstream text;
        text << "CLEO v" << CLEO_VERSION_STR;
#ifdef _DEBUG
        text << " ~r~~h~DEBUG";
#endif
        CFont::SetScale(sizeX, sizeY);
        CFont::PrintString(posX, posY - 15.0f * sizeY, text.str().c_str());

        // draw plugins / scripts text
        if (cs_count || plugin_count)
        {
            text.str(""); // clear
            if (plugin_count) text << plugin_count << (plugin_count > 1 ? " plugins" : " plugin");
            if (cs_count && plugin_count) text << " / ";
            if (cs_count) text << cs_count << (cs_count > 1 ? " scripts" : " script");

            posY += 15.0f * sizeY; // line feed
            sizeX *= subtextHeight;
            sizeY *= subtextHeight;
            CFont::SetScale(sizeX, sizeY);
            CFont::PrintString(posX, posY - 15.0f * sizeY, text.str().c_str());
        }
    }
}
