#pragma once

#include "stdafx.h"

class LightUtils
{
public:
    // Structure to hold light information for easier handling
    struct LightInfo
    {
        std::wstring type;
        ON_3dPoint location;
        ON_3dVector direction;
        double intensity;
        ON_Color color;
        bool isSpotLight;
        double innerAngle;  // For spot lights
        double outerAngle;  // For spot lights

        LightInfo() : intensity(0.0), isSpotLight(false), innerAngle(0.0), outerAngle(0.0) {}
    };

    // Main functions
    static std::vector<LightInfo> GetAllLights(CRhinoDoc* doc);
    static bool ExportLightsToFile(const std::vector<LightInfo>& lights, const std::wstring& filePath);
    static void PrintLightInventory(const std::vector<LightInfo>& lights);

    // Helper functions
    static std::wstring GetLightTypeString(ON::light_style style);
    static std::wstring DirectionToRotation(const ON_3dVector& direction);
    static std::wstring ColorToString(const ON_Color& color);
    static bool EnsureDirectoryExists(const std::wstring& filePath);

    // Constants
    static const std::wstring DEFAULT_EXPORT_PATH;
};