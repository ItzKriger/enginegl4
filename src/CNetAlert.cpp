#include "CNetAlert.h"
#include "CEngine.h"
#include "U_String.h"
#include "CLogger.h"
#include "CTerminal.h"
#include "CConsole.h"

void CNetAlert::V_Write(CBufferWrapper& wrapper)
{
    wrapper.Write(TextColor);
    wrapper.Write(BackgroundColor);
    wrapper.WriteLenString<std::uint16_t, std::u16string>(StringUtils::WstrToU16(String));
}

void CNetAlert::V_Read(CBufferWrapper& wrapper)
{
    wrapper.Read(TextColor);
    wrapper.Read(BackgroundColor);
    String = StringUtils::U16ToWstr(wrapper.ReadLenString<std::uint16_t, std::u16string>());
}

void CNetAlert::Process()
{
    CLogger* logger = CEngine::GetInstance()->Components.GetComponentTyped<CLogger>();
    CTerminal* terminal = CEngine::GetInstance()->Components.GetComponentTyped<CTerminal>();

    if(!logger) { return; }

    std::pair<CColor, CColor> oldcolors;
    if(terminal)
    {
        oldcolors = terminal->GetColor();

        auto txtColor = CColor(TextColor);
        auto bgColor = CColor(BackgroundColor);

        auto newcolors = std::make_pair(txtColor, bgColor);

        terminal->SetColor(newcolors);
    }

    logger->Out(String);
    if(terminal)
    {
        terminal->SetColor(oldcolors);
    }
}

LINK_NET_MESSAGE_TO_CLASS(CNetAlert, alert);