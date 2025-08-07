#pragma once

#include "stdafx.h"
#include "LightUtils.h"
#include <vector>
#include <string>
#include <iomanip>

class CLightEventWatcher : public CRhinoEventWatcher
{
public:
    void LightTableEvent(CRhinoEventWatcher::light_event event,
        const CRhinoLightTable& table, int lightIndex, const ON_Light* light) override;

private:
    // Helper methods
    static std::wstring GetLightEventTypeString(CRhinoEventWatcher::light_event event);
    static void SendLightDataToTCP(const std::vector<LightUtils::LightInfo>& lights,
        const std::wstring& eventType, int port);
    static std::wstring CreateLightDataJSON(const std::vector<LightUtils::LightInfo>& lights,
        const std::wstring& eventType);
    static std::wstring GetCurrentTimestamp();
    static std::string WStringToUTF8(const std::wstring& wstr);
};