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
#include <set>

/**
 * @brief Event watcher class for monitoring Rhino light table changes
 *
 * This class inherits from CRhinoEventWatcher and monitors light-related events
 * in Rhino (add, delete, modify operations). When events occur, it synchronizes
 * the light data with Unreal Engine via TCP communication.
 */
class CLightEventWatcher : public CRhinoEventWatcher
{
public:
    /**
     * @brief Structure to hold rotation data in Rhino format
     *
     * Represents rotation as pitch, yaw, roll angles in degrees.
     * This format is directly compatible with Unreal Engine's FRotator.
     */
    struct FRhinoRotation
    {
        double pitch;  // Rotation around Y axis (elevation)
        double yaw;    // Rotation around Z axis (azimuth)
        double roll;   // Rotation around X axis (twist)

        FRhinoRotation() : pitch(0.0), yaw(0.0), roll(0.0) {}
    };

    /**
     * @brief Main event handler for light table changes
     *
     * Called automatically by Rhino when lights are added, deleted, or modified.
     * Collects all light data and sends it to Unreal Engine.
     */
    virtual void LightTableEvent(CRhinoEventWatcher::light_event event,
        const CRhinoLightTable& table, int lightIndex, const ON_Light* light) override;

private:
    // Blacklist to track deleted lights by their serial number
    static std::set<unsigned int> m_deletedLightsBlacklist;

    // Event processing functions
    static std::wstring GetLightEventTypeString(CRhinoEventWatcher::light_event event);
    static double GetModelUnitScaleToMeters(CRhinoDoc* doc);
    static void ConvertLightsToMeters(std::vector<LightUtils::LightInfo>& lights, double unitScale);

    // Blacklist management functions
    static void AddToBlacklist(unsigned int lightSerialNumber);
    static void RemoveFromBlacklist(unsigned int lightSerialNumber);
    static bool IsBlacklisted(unsigned int lightSerialNumber);
    static std::vector<LightUtils::LightInfo> FilterBlacklistedLights(const std::vector<LightUtils::LightInfo>& allLights, CRhinoDoc* doc);

    // Network communication functions
    static void SendLightDataToTCP(const std::vector<LightUtils::LightInfo>& lights,
        const std::wstring& eventType, int port);
    static std::wstring CreateLightDataJSON(const std::vector<LightUtils::LightInfo>& lights,
        const std::wstring& eventType);

    // Utility functions
    static FRhinoRotation DirectionToRhinoRotation(const ON_3dVector& direction);
    static std::string WStringToUTF8(const std::wstring& wstr);
};