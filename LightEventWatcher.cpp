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
#include "LightEventWatcher.h"
#include "LightUtils.h"
#include "rhinoSdkApp.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sstream>
#include <thread>
#include <iomanip>
#pragma comment(lib, "ws2_32.lib")

// Constants for TCP communication
namespace {
    constexpr int DEFAULT_TCP_PORT = 5173;
    constexpr DWORD TCP_TIMEOUT_MS = 5000;
    constexpr const char* LOCALHOST_IP = "127.0.0.1";
}

// Static member initialization
std::set<unsigned int> CLightEventWatcher::m_deletedLightsBlacklist;

/**
 * @brief Handles light table events and broadcasts light data via TCP
 *
 * This function is called whenever a light is added, deleted, undeleted, or modified in Rhino.
 * It manages a blacklist of deleted lights, collects all active lights, converts their coordinates
 * to meters, and sends the data to Unreal Engine via TCP connection.
 *
 * @param event The type of light event that occurred
 * @param table Reference to the light table
 * @param lightIndex Index of the affected light
 * @param light Pointer to the light object (may be null for delete events)
 */
void CLightEventWatcher::LightTableEvent(CRhinoEventWatcher::light_event event,
    const CRhinoLightTable& table, int lightIndex, const ON_Light* light)
{
    try
    {
        // Validate active document exists
        CRhinoDoc* doc = RhinoApp().ActiveDoc();
        if (!doc)
        {
            RhinoApp().Print(L"Warning: No active document found for light event.\n");
            return;
        }

        // Handle blacklist management based on event type
        if (event == CRhinoEventWatcher::light_event::light_deleted && lightIndex >= 0)
        {
            // Get the light that's being deleted and add to blacklist
            const CRhinoLight* rhinoLight = &table[lightIndex];
            if (rhinoLight)
            {
                unsigned int serialNumber = rhinoLight->Attributes().m_uuid.Data1; // Using UUID as unique identifier
                AddToBlacklist(serialNumber);
                RhinoApp().Print(L"Added light (Serial: %u) to blacklist due to deletion.\n", serialNumber);
            }
        }
        else if (event == CRhinoEventWatcher::light_event::light_undeleted && lightIndex >= 0)
        {
            // Get the light that's being undeleted and remove from blacklist
            const CRhinoLight* rhinoLight = &table[lightIndex];
            if (rhinoLight)
            {
                unsigned int serialNumber = rhinoLight->Attributes().m_uuid.Data1;
                RemoveFromBlacklist(serialNumber);
                RhinoApp().Print(L"Removed light (Serial: %u) from blacklist due to undeletion.\n", serialNumber);
            }
        }

        // Get model unit scale factor for conversion to meters (Unreal's standard unit)
        double unitScale = GetModelUnitScaleToMeters(doc);

        // Retrieve all lights from the document (including deleted ones that are still in table)
        std::vector<LightUtils::LightInfo> allLights = LightUtils::GetAllLights(doc);

        // Filter out blacklisted (deleted) lights
        std::vector<LightUtils::LightInfo> activeLights = FilterBlacklistedLights(allLights, doc);

        // Convert all active light coordinates to meters for Unreal compatibility
        ConvertLightsToMeters(activeLights, unitScale);

        // Log event information for debugging
        std::wstring eventType = GetLightEventTypeString(event);
        RhinoApp().Print(L"Light Event: %s (Total lights in table: %d, Active lights after filtering: %d, Unit scale: %.6f)\n",
            eventType.c_str(), static_cast<int>(allLights.size()), static_cast<int>(activeLights.size()), unitScale);

        // Send light data to Unreal Engine via TCP in background thread
        // This prevents blocking the UI while network communication occurs
        std::thread tcpThread([activeLights, eventType]() {
            SendLightDataToTCP(activeLights, eventType, DEFAULT_TCP_PORT);
            });
        tcpThread.detach();

        // Export to file as backup (optional safety measure)
        if (LightUtils::ExportLightsToFile(activeLights, LightUtils::DEFAULT_EXPORT_PATH))
        {
            RhinoApp().Print(L"Active light data exported to backup file successfully.\n");
        }
    }
    catch (const std::exception& e)
    {
        // Convert exception message to wide string for Rhino console
        std::string errorMsg = e.what();
        std::wstring wErrorMsg(errorMsg.begin(), errorMsg.end());
        RhinoApp().Print(L"Error: Standard exception in light event handler: %s\n",
            wErrorMsg.c_str());
    }
    catch (...)
    {
        RhinoApp().Print(L"Error: Unknown exception occurred in light event handler.\n");
    }
}

/**
 * @brief Adds a light to the deletion blacklist
 *
 * @param lightSerialNumber Unique identifier for the light
 */
void CLightEventWatcher::AddToBlacklist(unsigned int lightSerialNumber)
{
    m_deletedLightsBlacklist.insert(lightSerialNumber);
}

/**
 * @brief Removes a light from the deletion blacklist
 *
 * @param lightSerialNumber Unique identifier for the light
 */
void CLightEventWatcher::RemoveFromBlacklist(unsigned int lightSerialNumber)
{
    m_deletedLightsBlacklist.erase(lightSerialNumber);
}

/**
 * @brief Checks if a light is blacklisted (deleted)
 *
 * @param lightSerialNumber Unique identifier for the light
 * @return True if the light is blacklisted
 */
bool CLightEventWatcher::IsBlacklisted(unsigned int lightSerialNumber)
{
    return m_deletedLightsBlacklist.find(lightSerialNumber) != m_deletedLightsBlacklist.end();
}

/**
 * @brief Filters out blacklisted lights from the complete lights list
 *
 * @param allLights Vector containing all lights from the document
 * @param doc Pointer to the Rhino document
 * @return Vector containing only non-blacklisted lights
 */
std::vector<LightUtils::LightInfo> CLightEventWatcher::FilterBlacklistedLights(
    const std::vector<LightUtils::LightInfo>& allLights, CRhinoDoc* doc)
{
    std::vector<LightUtils::LightInfo> filteredLights;

    if (!doc)
    {
        return filteredLights;
    }

    // Get all lights from the light table to correlate with our light info
    ON_SimpleArray<const CRhinoLight*> rhinoLights;
    doc->m_light_table.GetSortedList(rhinoLights);

    // Filter out blacklisted lights
    for (size_t i = 0; i < allLights.size() && i < static_cast<size_t>(rhinoLights.Count()); ++i)
    {
        const CRhinoLight* rhinoLight = rhinoLights[static_cast<int>(i)];
        if (rhinoLight)
        {
            unsigned int serialNumber = rhinoLight->Attributes().m_uuid.Data1;

            // Only include lights that are not blacklisted
            if (!IsBlacklisted(serialNumber))
            {
                filteredLights.push_back(allLights[i]);
            }
        }
    }

    return filteredLights;
}

/**
 * @brief Converts light event enum to human-readable string
 *
 * @param event The light event type
 * @return Wide string representation of the event
 */
std::wstring CLightEventWatcher::GetLightEventTypeString(CRhinoEventWatcher::light_event event)
{
    switch (event)
    {
    case CRhinoEventWatcher::light_event::light_added:
        return L"Light Added";
    case CRhinoEventWatcher::light_event::light_deleted:
        return L"Light Deleted";
    case CRhinoEventWatcher::light_event::light_undeleted:
        return L"Light Undeleted";
    case CRhinoEventWatcher::light_event::light_modified:
        return L"Light Modified";
    default:
        return L"Unknown Light Event";
    }
}

/**
 * @brief Gets the scale factor to convert model units to meters
 *
 * Unreal Engine uses meters as its standard unit, so we need to convert
 * Rhino's model units to meters for proper scaling in UE.
 *
 * @param doc Pointer to the Rhino document
 * @return Scale factor (model units per meter)
 */
double CLightEventWatcher::GetModelUnitScaleToMeters(CRhinoDoc* doc)
{
    if (!doc)
        return 1.0;

    ON::LengthUnitSystem modelUnits = doc->Properties().ModelUnits().UnitSystem();

    // Convert from model units to meters
    switch (modelUnits)
    {
    case ON::LengthUnitSystem::Millimeters:
        return 0.001;  // 1000 mm = 1 m
    case ON::LengthUnitSystem::Centimeters:
        return 0.01;   // 100 cm = 1 m
    case ON::LengthUnitSystem::Meters:
        return 1.0;    // 1 m = 1 m
    case ON::LengthUnitSystem::Kilometers:
        return 1000.0; // 0.001 km = 1 m
    case ON::LengthUnitSystem::Inches:
        return 0.0254; // 39.37 in = 1 m
    case ON::LengthUnitSystem::Feet:
        return 0.3048; // 3.281 ft = 1 m
    case ON::LengthUnitSystem::Yards:
        return 0.9144; // 1.094 yd = 1 m
    case ON::LengthUnitSystem::Miles:
        return 1609.344; // 0.000621 mi = 1 m
    default:
        return 1.0;    // Default to no conversion
    }
}

/**
 * @brief Converts all light positions to meters for Unreal compatibility
 *
 * Only positions need scaling - direction vectors are unit vectors and don't need scaling.
 * Intensity and color values remain unchanged as they are not spatial measurements.
 *
 * @param lights Reference to vector of light info structures
 * @param unitScale Scale factor to convert to meters
 */
void CLightEventWatcher::ConvertLightsToMeters(std::vector<LightUtils::LightInfo>& lights, double unitScale)
{
    for (auto& light : lights)
    {
        // Convert position coordinates to meters
        light.location.x *= unitScale;
        light.location.y *= unitScale;
        light.location.z *= unitScale;

        // Note: Direction vectors are unit vectors, so they don't need scaling
        // Intensity and color values remain unchanged as they are not spatial measurements
    }
}

/**
 * @brief Sends light data to Unreal Engine via TCP connection
 *
 * Creates a simplified JSON payload with light data and sends it asynchronously
 * to prevent blocking the main thread. Handles all socket operations and cleanup.
 *
 * @param lights Vector of light information (already converted to meters)
 * @param eventType String describing the event type
 * @param port TCP port number to connect to (should match Unreal's listener)
 */
void CLightEventWatcher::SendLightDataToTCP(const std::vector<LightUtils::LightInfo>& lights,
    const std::wstring& eventType, int port)
{
    WSADATA wsaData;
    SOCKET connectSocket = INVALID_SOCKET;

    try
    {
        // Initialize Winsock library for network operations
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0)
        {
            return; // Failed to initialize Winsock
        }

        // Create TCP socket for communication
        connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (connectSocket == INVALID_SOCKET)
        {
            WSACleanup();
            return;
        }

        // Configure server address structure (connecting to local Unreal instance)
        sockaddr_in serverAddr = {};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(static_cast<u_short>(port));

        // Convert IP address string to binary form
        if (inet_pton(AF_INET, LOCALHOST_IP, &serverAddr.sin_addr) <= 0)
        {
            closesocket(connectSocket);
            WSACleanup();
            return;
        }

        // Set socket timeout to prevent hanging if Unreal isn't responding
        setsockopt(connectSocket, SOL_SOCKET, SO_SNDTIMEO,
            reinterpret_cast<const char*>(&TCP_TIMEOUT_MS), sizeof(TCP_TIMEOUT_MS));

        // Attempt connection to Unreal Engine's TCP listener
        result = connect(connectSocket, reinterpret_cast<SOCKADDR*>(&serverAddr), sizeof(serverAddr));
        if (result == SOCKET_ERROR)
        {
            closesocket(connectSocket);
            WSACleanup();
            return;
        }

        // Create simplified JSON payload with light data including rotation
        std::wstring jsonData = CreateLightDataJSON(lights, eventType);
        std::string utf8Data = WStringToUTF8(jsonData);

        // Send data to Unreal Engine
        result = send(connectSocket, utf8Data.c_str(), static_cast<int>(utf8Data.length()), 0);

        // Clean up connection
        closesocket(connectSocket);
        WSACleanup();

        // Note: Can't use RhinoApp().Print() here as this runs in a separate thread
    }
    catch (...)
    {
        // Ensure cleanup on any exception to prevent resource leaks
        if (connectSocket != INVALID_SOCKET)
        {
            closesocket(connectSocket);
        }
        WSACleanup();
    }
}

/**
 * @brief Creates simplified JSON representation of light data with rotation
 *
 * Creates a streamlined JSON structure optimized for Unreal Engine consumption.
 * Includes rotation data directly instead of direction vectors to avoid conversion issues.
 *
 * @param lights Vector of light information (already converted to meters)
 * @param eventType String describing the event type
 * @return JSON string containing all light data
 */
std::wstring CLightEventWatcher::CreateLightDataJSON(const std::vector<LightUtils::LightInfo>& lights,
    const std::wstring& eventType)
{
    std::wstringstream json;

    // JSON root object - simplified structure
    json << L"{\n";
    json << L"  \"event\": \"" << eventType << L"\",\n";
    json << L"  \"lightCount\": " << lights.size() << L",\n";
    json << L"  \"lights\": [\n";

    // Serialize each light with rotation data
    for (size_t i = 0; i < lights.size(); ++i)
    {
        const auto& light = lights[i];

        json << L"    {\n";
        json << L"      \"id\": " << i << L",\n";
        json << L"      \"type\": \"" << light.type << L"\",\n";

        // Position in meters (already converted)
        json << L"      \"location\": {\n";
        json << L"        \"x\": " << std::fixed << std::setprecision(6) << light.location.x << L",\n";
        json << L"        \"y\": " << std::fixed << std::setprecision(6) << light.location.y << L",\n";
        json << L"        \"z\": " << std::fixed << std::setprecision(6) << light.location.z << L"\n";
        json << L"      },\n";

        // Calculate and send rotation directly instead of direction vector
        // This avoids complex vector-to-rotation conversion in Unreal
        FRhinoRotation rotation = DirectionToRhinoRotation(light.direction);
        json << L"      \"rotation\": {\n";
        json << L"        \"pitch\": " << std::fixed << std::setprecision(3) << rotation.pitch << L",\n";
        json << L"        \"yaw\": " << std::fixed << std::setprecision(3) << rotation.yaw << L",\n";
        json << L"        \"roll\": " << std::fixed << std::setprecision(3) << rotation.roll << L"\n";
        json << L"      },\n";

        json << L"      \"intensity\": " << light.intensity << L",\n";

        // RGB color values (0-255 range)
        json << L"      \"color\": {\n";
        json << L"        \"r\": " << static_cast<int>(light.color.Red()) << L",\n";
        json << L"        \"g\": " << static_cast<int>(light.color.Green()) << L",\n";
        json << L"        \"b\": " << static_cast<int>(light.color.Blue()) << L"\n";
        json << L"      }";

        // Optional spotlight parameters
        if (light.isSpotLight)
        {
            json << L",\n";
            json << L"      \"spotLight\": {\n";
            json << L"        \"innerAngle\": " << light.innerAngle << L",\n";
            json << L"        \"outerAngle\": " << light.outerAngle << L"\n";
            json << L"      }";
        }

        json << L"\n    }";

        // Add comma if not the last element
        if (i < lights.size() - 1)
        {
            json << L",";
        }
        json << L"\n";
    }

    json << L"  ]\n";
    json << L"}";

    return json.str();
}

/**
 * @brief Converts direction vector to rotation suitable for Unreal Engine
 *
 * Converts a 3D direction vector to pitch, yaw, roll rotation values.
 * Uses proper coordinate system conversion for Rhino to Unreal compatibility.
 *
 * @param direction Direction vector from Rhino light
 * @return Rotation structure with pitch, yaw, roll values in degrees
 */
CLightEventWatcher::FRhinoRotation CLightEventWatcher::DirectionToRhinoRotation(const ON_3dVector& direction)
{
    FRhinoRotation rotation;

    // Normalize the direction vector to ensure unit length
    ON_3dVector normalized = direction;
    normalized.Unitize();

    // Calculate pitch (elevation angle) - rotation around Y axis
    // Pitch is the angle between the direction and the XY plane
    rotation.pitch = asin(-normalized.z) * 180.0 / ON_PI;

    // Calculate yaw (azimuth angle) - rotation around Z axis
    // Yaw is the angle in the XY plane from the positive X axis
    rotation.yaw = atan2(normalized.y, normalized.x) * 180.0 / ON_PI;

    // Roll is typically 0 for lights (no twist around the direction axis)
    rotation.roll = 0.0;

    return rotation;
}

/**
 * @brief Converts wide string to UTF-8 encoded string for network transmission
 *
 * @param wstr Wide string to convert
 * @return UTF-8 encoded string suitable for network transmission
 */
std::string CLightEventWatcher::WStringToUTF8(const std::wstring& wstr)
{
    if (wstr.empty())
        return std::string();

    // Calculate required buffer size
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()),
        nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0)
        return std::string();

    // Perform the conversion
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()),
        &strTo[0], size_needed, nullptr, nullptr);

    return strTo;
}