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
#include "stdafx.h"
#include "LightUtils.h"
#include <fstream>
#include <sstream>
#include <vector>

const std::wstring LightUtils::DEFAULT_EXPORT_PATH = L"C:/ProgramData/RhinoLightSync/Lights.txt";

std::vector<LightUtils::LightInfo> LightUtils::GetAllLights(CRhinoDoc* doc)
{
    std::vector<LightInfo> lightInfos;

    if (nullptr == doc)
    {
        return lightInfos; // Return empty vector
    }

    try
    {
        // Retrieve all lights from the document's light table
        ON_SimpleArray<const CRhinoLight*> lights;
        doc->m_light_table.GetSortedList(lights);
        const int lightCount = lights.Count();

        // Convert each light to LightInfo structure
        for (int i = 0; i < lightCount; ++i)
        {
            const auto& light = lights[i]->Light();
            if (!light.m_bOn)
                continue;
            LightInfo info;
            info.type = GetLightTypeString(light.Style());
            info.location = light.Location();
            info.direction = light.Direction();
            info.intensity = light.Intensity();
            info.color = light.Diffuse();
            info.isSpotLight = light.IsSpotLight();

            if (info.isSpotLight)
            {
                info.outerAngle = light.HotSpot() * (180.0 / 3.14159265358979323846);
                info.innerAngle = light.SpotAngleDegrees();
            }

            lightInfos.push_back(info);
        }
    }
    catch (...)
    {
        // Return whatever we managed to collect
    }

    return lightInfos;
}

bool LightUtils::ExportLightsToFile(const std::vector<LightInfo>& lights, const std::wstring& filePath)
{
    try
    {
        // Ensure output directory exists
        if (!EnsureDirectoryExists(filePath))
        {
            return false;
        }

        // Open file for writing (overwrite existing)
        std::wofstream outFile(filePath, std::ios::out | std::ios::trunc);
        if (!outFile.is_open())
        {
            return false;
        }

        // Write header comment
        outFile << L"# RhinoLightSync Export File" << std::endl;
        outFile << L"# Format: <Type> <Location> <Rotation> <Intensity> <Color> [InnerAngle OuterAngle]" << std::endl;
        outFile << L"# Total Lights: " << lights.size() << std::endl << std::endl;

        // Export each light
        for (const auto& lightInfo : lights)
        {
            std::wstring rotationString = DirectionToRotation(lightInfo.direction);
            std::wstring colorString = ColorToString(lightInfo.color);

            // Write formatted line: Type Location Rotation Intensity Color
            outFile << lightInfo.type << L" "
                << L"(" << lightInfo.location.x << L"," << lightInfo.location.y << L"," << lightInfo.location.z << L") "
                << rotationString << L" "
                << lightInfo.intensity << L" "
                << colorString;

            // Add spot light angles if applicable
            if (lightInfo.isSpotLight)
            {
                outFile << L" " << lightInfo.innerAngle << L"° " << lightInfo.outerAngle << L"°";
            }

            outFile << std::endl;
        }

        outFile.close();
        return true;
    }
    catch (...)
    {
        return false;
    }
}

void LightUtils::PrintLightInventory(const std::vector<LightInfo>& lights)
{
    // Display summary information
    RhinoApp().Print(L"=== Light Inventory Report ===\n");
    RhinoApp().Print(L"Scene contains %d light(s):\n\n", (int)lights.size());

    // Process each light and display its properties
    for (size_t i = 0; i < lights.size(); ++i)
    {
        const auto& lightInfo = lights[i];

        // Display light information in console
        RhinoApp().Print(L"Light %d:\n", (int)(i + 1));
        RhinoApp().Print(L"  Type: %s\n", lightInfo.type.c_str());
        RhinoApp().Print(L"  Position: (%.3f, %.3f, %.3f)\n",
            lightInfo.location.x, lightInfo.location.y, lightInfo.location.z);
        RhinoApp().Print(L"  Direction: (%.3f, %.3f, %.3f)\n",
            lightInfo.direction.x, lightInfo.direction.y, lightInfo.direction.z);
        RhinoApp().Print(L"  Intensity: %.3f\n", lightInfo.intensity);
        RhinoApp().Print(L"  Color: %s\n", ColorToString(lightInfo.color).c_str());

        // Display spot light specific properties
        if (lightInfo.isSpotLight)
        {
            RhinoApp().Print(L"  Inner Angle: %.2f°\n", lightInfo.innerAngle);
            RhinoApp().Print(L"  Outer Angle: %.2f°\n", lightInfo.outerAngle);
        }

        RhinoApp().Print(L"\n");
    }

    RhinoApp().Print(L"=== End of Light Report ===\n");
}

std::wstring LightUtils::GetLightTypeString(ON::light_style style)
{
    switch (style)
    {
    case ON::camera_directional_light:
    case ON::world_directional_light:
        return L"Directional";

    case ON::camera_point_light:
    case ON::world_point_light:
        return L"Point";

    case ON::camera_spot_light:
    case ON::world_spot_light:
        return L"Spot";

    case ON::ambient_light:
        return L"Ambient";

    default:
        return L"Unknown";
    }
}

bool LightUtils::EnsureDirectoryExists(const std::wstring& filePath)
{
    // Extract directory path from full file path
    size_t lastSlash = filePath.find_last_of(L"/\\");
    if (lastSlash == std::wstring::npos)
    {
        return true; // No directory specified, use current directory
    }

    std::wstring dirPath = filePath.substr(0, lastSlash);

    // Use Windows API to create directory structure
    DWORD attributes = GetFileAttributesW(dirPath.c_str());

    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        // Directory doesn't exist, attempt to create it
        if (!CreateDirectoryW(dirPath.c_str(), nullptr))
        {
            DWORD error = GetLastError();
            if (error != ERROR_ALREADY_EXISTS)
            {
                return false;
            }
        }
    }
    else if (!(attributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        // Path exists but is not a directory
        return false;
    }

    return true;
}

std::wstring LightUtils::DirectionToRotation(const ON_3dVector& direction)
{
    // Calculate rotation angles from direction vector
    ON_3dVector normalized = direction;
    normalized.Unitize();

    // Calculate elevation (pitch) and azimuth (yaw) angles
    double elevation = asin(normalized.z) * 180.0 / ON_PI;
    double azimuth = atan2(normalized.y, normalized.x) * 180.0 / ON_PI;

    // Format as string
    std::wstringstream ss;
    ss << L"(" << azimuth << L"°, " << elevation << L"°, 0.00°)";
    return ss.str();
}

std::wstring LightUtils::ColorToString(const ON_Color& color)
{
    std::wstringstream ss;
    ss << L"RGB(" << (int)color.Red() << L"," << (int)color.Green() << L"," << (int)color.Blue() << L")";
    return ss.str();
}