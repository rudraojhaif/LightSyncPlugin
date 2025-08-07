// Copyright (c) 2025 Rudra Ojha
// All rights reserved.
//
// This source code is provided for educational and reference purposes only.
// Redistribution, modification, or use of this code in any commercial or private
// product is strictly prohibited without explicit written permission from the author.
//
// Unauthorized use in any software or plugin distributed to end-users,
// whether open-source or commercial, is not allowed.
//
// Contact: rudraojhaif@gmail.com for licensing inquiries.
#pragma once

#include "stdafx.h"
#include "LightUtils.h"
#include <vector>
#include <string>
#include <iomanip>

/**
 * @brief Event watcher class for monitoring light table changes in Rhino
 *
 * This class handles light events (add, delete, modify) and broadcasts
 * the light data via TCP. All exported data is converted to meters regardless
 * of the model's unit system.
 */
class CLightEventWatcher : public CRhinoEventWatcher
{
public:
    /**
     * @brief Handles light table events
     * @param event Type of light event that occurred
     * @param table Reference to the light table
     * @param lightIndex Index of the affected light
     * @param light Pointer to the light object (may be null for delete events)
     */
    void LightTableEvent(CRhinoEventWatcher::light_event event,
        const CRhinoLightTable& table, int lightIndex, const ON_Light* light) override;

private:
    // Event handling helpers
    static std::wstring GetLightEventTypeString(CRhinoEventWatcher::light_event event);

    // Unit conversion methods
    static double GetModelUnitScaleToMeters(CRhinoDoc* doc);
    static void ConvertLightsToMeters(std::vector<LightUtils::LightInfo>& lights, double unitScale);

    // TCP communication methods
    static void SendLightDataToTCP(const std::vector<LightUtils::LightInfo>& lights,
        const std::wstring& eventType, int port);

    // JSON serialization methods
    static std::wstring CreateLightDataJSON(const std::vector<LightUtils::LightInfo>& lights,
        const std::wstring& eventType);

    // Utility methods
    static std::wstring GetCurrentTimestamp();
    static std::string WStringToUTF8(const std::wstring& wstr);
};