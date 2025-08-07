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

#pragma comment(lib, "ws2_32.lib")

// Constants
namespace {
    constexpr int DEFAULT_TCP_PORT = 5173;
    constexpr DWORD TCP_TIMEOUT_MS = 5000;
    constexpr const char* LOCALHOST_IP = "127.0.0.1";
}

/**
 * @brief Handles light table events and broadcasts light data via TCP
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
        // Validate active document
        CRhinoDoc* doc = RhinoApp().ActiveDoc();
        if (!doc)
        {
            RhinoApp().Print(L"Warning: No active document found for light event.\n");
            return;
        }

        // Get model unit scale factor for conversion to meters
        double unitScale = GetModelUnitScaleToMeters(doc);

        // Retrieve all lights from the document
        std::vector<LightUtils::LightInfo> lights = LightUtils::GetAllLights(doc);

        // Convert all light data to meters
        ConvertLightsToMeters(lights, unitScale);

        // Log event information
        std::wstring eventType = GetLightEventTypeString(event);
        RhinoApp().Print(L"Light Event: %s (Total lights: %d, Unit scale: %.6f)\n",
            eventType.c_str(), static_cast<int>(lights.size()), unitScale);

        // Broadcast light data via TCP in background thread
        std::thread tcpThread([lights, eventType]() {
            SendLightDataToTCP(lights, eventType, DEFAULT_TCP_PORT);
            });
        tcpThread.detach();

        // Export to file as backup (optional)
        if (LightUtils::ExportLightsToFile(lights, LightUtils::DEFAULT_EXPORT_PATH))
        {
            RhinoApp().Print(L"Light data exported to file successfully.\n");
        }
    }
    catch (const std::exception& e)
    {
        // Convert exception message to wide string
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
 * @brief Converts light event enum to human-readable string
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
 * @brief Converts all light positions and directions to meters
 * @param lights Reference to vector of light info structures
 * @param unitScale Scale factor to convert to meters
 */
void CLightEventWatcher::ConvertLightsToMeters(std::vector<LightUtils::LightInfo>& lights, double unitScale)
{
    for (auto& light : lights)
    {
        // Convert position to meters
        light.location.x *= unitScale;
        light.location.y *= unitScale;
        light.location.z *= unitScale;

        // Direction vectors are unit vectors, so they don't need scaling
        // Intensity and color values remain unchanged as they are not spatial measurements
    }
}

/**
 * @brief Sends light data to TCP server asynchronously
 * @param lights Vector of light information
 * @param eventType String describing the event type
 * @param port TCP port number to connect to
 */
void CLightEventWatcher::SendLightDataToTCP(const std::vector<LightUtils::LightInfo>& lights,
    const std::wstring& eventType, int port)
{
    WSADATA wsaData;
    SOCKET connectSocket = INVALID_SOCKET;

    try
    {
        // Initialize Winsock library
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0)
        {
            return; // Failed to initialize Winsock
        }

        // Create TCP socket
        connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (connectSocket == INVALID_SOCKET)
        {
            WSACleanup();
            return;
        }

        // Configure server address structure
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

        // Set socket timeout to prevent hanging
        setsockopt(connectSocket, SOL_SOCKET, SO_SNDTIMEO,
            reinterpret_cast<const char*>(&TCP_TIMEOUT_MS), sizeof(TCP_TIMEOUT_MS));

        // Attempt connection to server
        result = connect(connectSocket, reinterpret_cast<SOCKADDR*>(&serverAddr), sizeof(serverAddr));
        if (result == SOCKET_ERROR)
        {
            closesocket(connectSocket);
            WSACleanup();
            return;
        }

        // Create JSON payload with light data
        std::wstring jsonData = CreateLightDataJSON(lights, eventType);
        std::string utf8Data = WStringToUTF8(jsonData);

        // Send data to server
        result = send(connectSocket, utf8Data.c_str(), static_cast<int>(utf8Data.length()), 0);

        // Clean up connection
        closesocket(connectSocket);
        WSACleanup();

        // Note: Can't use RhinoApp().Print() here as this runs in a separate thread
    }
    catch (...)
    {
        // Ensure cleanup on any exception
        if (connectSocket != INVALID_SOCKET)
        {
            closesocket(connectSocket);
        }
        WSACleanup();
    }
}

/**
 * @brief Creates JSON representation of light data
 * @param lights Vector of light information (already converted to meters)
 * @param eventType String describing the event type
 * @return JSON string containing all light data
 */
std::wstring CLightEventWatcher::CreateLightDataJSON(const std::vector<LightUtils::LightInfo>& lights,
    const std::wstring& eventType)
{
    std::wstringstream json;

    // JSON root object
    json << L"{\n";
    json << L"  \"event\": \"" << eventType << L"\",\n";
    json << L"  \"timestamp\": \"" << GetCurrentTimestamp() << L"\",\n";
    json << L"  \"units\": \"meters\",\n";  // Always meters after conversion
    json << L"  \"lightCount\": " << lights.size() << L",\n";
    json << L"  \"lights\": [\n";

    // Serialize each light
    for (size_t i = 0; i < lights.size(); ++i)
    {
        const auto& light = lights[i];

        json << L"    {\n";
        json << L"      \"id\": " << i << L",\n";
        json << L"      \"type\": \"" << light.type << L"\",\n";

        // Position (in meters)
        json << L"      \"location\": {\n";
        json << L"        \"x\": " << std::fixed << std::setprecision(6) << light.location.x << L",\n";
        json << L"        \"y\": " << std::fixed << std::setprecision(6) << light.location.y << L",\n";
        json << L"        \"z\": " << std::fixed << std::setprecision(6) << light.location.z << L"\n";
        json << L"      },\n";

        // Direction (unit vector)
        json << L"      \"direction\": {\n";
        json << L"        \"x\": " << std::fixed << std::setprecision(6) << light.direction.x << L",\n";
        json << L"        \"y\": " << std::fixed << std::setprecision(6) << light.direction.y << L",\n";
        json << L"        \"z\": " << std::fixed << std::setprecision(6) << light.direction.z << L"\n";
        json << L"      },\n";

        json << L"      \"intensity\": " << light.intensity << L",\n";

        // RGB color values
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
 * @brief Generates ISO 8601 timestamp string
 * @return Current local time in ISO format
 */
std::wstring CLightEventWatcher::GetCurrentTimestamp()
{
    SYSTEMTIME st;
    GetLocalTime(&st);

    std::wstringstream ss;
    ss << st.wYear << L"-"
        << std::setfill(L'0') << std::setw(2) << st.wMonth << L"-"
        << std::setfill(L'0') << std::setw(2) << st.wDay << L"T"
        << std::setfill(L'0') << std::setw(2) << st.wHour << L":"
        << std::setfill(L'0') << std::setw(2) << st.wMinute << L":"
        << std::setfill(L'0') << std::setw(2) << st.wSecond;

    return ss.str();
}

/**
 * @brief Converts wide string to UTF-8 encoded string
 * @param wstr Wide string to convert
 * @return UTF-8 encoded string
 */
std::string CLightEventWatcher::WStringToUTF8(const std::wstring& wstr)
{
    if (wstr.empty())
        return std::string();

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()),
        nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0)
        return std::string();

    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()),
        &strTo[0], size_needed, nullptr, nullptr);

    return strTo;
}