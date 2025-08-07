#include "stdafx.h"
#include "LightEventWatcher.h"
#include "LightUtils.h"
#include "rhinoSdkApp.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sstream>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

void CLightEventWatcher::LightTableEvent(CRhinoEventWatcher::light_event event,
    const CRhinoLightTable& table, int lightIndex, const ON_Light* light)
{
    try
    {
        // Get the active document
        CRhinoDoc* doc = RhinoApp().ActiveDoc();
        if (!doc)
        {
            RhinoApp().Print(L"Warning: No active document found for light event.\n");
            return;
        }

        // Get all lights using the utility class
        std::vector<LightUtils::LightInfo> lights = LightUtils::GetAllLights(doc);

        // Print event information
        std::wstring eventType = GetLightEventTypeString(event);
        RhinoApp().Print(L"Light Event: %s (Total lights: %d)\n",
            eventType.c_str(), (int)lights.size());

        // Send light data to TCP port 5173
        std::thread tcpThread([lights, eventType]() {
            SendLightDataToTCP(lights, eventType, 5173);
            });
        tcpThread.detach(); // Run in background

        // Also export to file (optional)
        if (LightUtils::ExportLightsToFile(lights, LightUtils::DEFAULT_EXPORT_PATH))
        {
            RhinoApp().Print(L"Light data exported to file successfully.\n");
        }
    }
    catch (const std::exception& e)
    {
        RhinoApp().Print(L"Error: Exception occurred in light event handler.\n");
    }
    catch (...)
    {
        RhinoApp().Print(L"Error: Unknown exception occurred in light event handler.\n");
    }
}

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

void CLightEventWatcher::SendLightDataToTCP(const std::vector<LightUtils::LightInfo>& lights,
    const std::wstring& eventType, int port)
{
    WSADATA wsaData;
    SOCKET connectSocket = INVALID_SOCKET;

    try
    {
        // Initialize Winsock
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0)
        {
            return;
        }

        // Create socket
        connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (connectSocket == INVALID_SOCKET)
        {
            WSACleanup();
            return;
        }

        // Set up server address
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);

        // Connect to localhost
        if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0)
        {
            closesocket(connectSocket);
            WSACleanup();
            return;
        }

        // Set socket timeout (5 seconds)
        DWORD timeout = 5000;
        setsockopt(connectSocket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

        // Connect to server
        result = connect(connectSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
        if (result == SOCKET_ERROR)
        {
            closesocket(connectSocket);
            WSACleanup();
            return;
        }

        // Prepare JSON-like data
        std::wstring jsonData = CreateLightDataJSON(lights, eventType);

        // Convert to UTF-8
        std::string utf8Data = WStringToUTF8(jsonData);

        // Send data
        result = send(connectSocket, utf8Data.c_str(), (int)utf8Data.length(), 0);

        // Close connection
        closesocket(connectSocket);
        WSACleanup();

        if (result != SOCKET_ERROR)
        {
            // Print success message in main thread
            // Note: This runs in a separate thread, so we can't use RhinoApp().Print() here
        }
    }
    catch (...)
    {
        if (connectSocket != INVALID_SOCKET)
        {
            closesocket(connectSocket);
        }
        WSACleanup();
    }
}

std::wstring CLightEventWatcher::CreateLightDataJSON(const std::vector<LightUtils::LightInfo>& lights,
    const std::wstring& eventType)
{
    std::wstringstream json;

    json << L"{\n";
    json << L"  \"event\": \"" << eventType << L"\",\n";
    json << L"  \"timestamp\": \"" << GetCurrentTimestamp() << L"\",\n";
    json << L"  \"lightCount\": " << lights.size() << L",\n";
    json << L"  \"lights\": [\n";

    for (size_t i = 0; i < lights.size(); ++i)
    {
        const auto& light = lights[i];

        json << L"    {\n";
        json << L"      \"id\": " << i << L",\n";
        json << L"      \"type\": \"" << light.type << L"\",\n";
        json << L"      \"location\": {\n";
        json << L"        \"x\": " << light.location.x << L",\n";
        json << L"        \"y\": " << light.location.y << L",\n";
        json << L"        \"z\": " << light.location.z << L"\n";
        json << L"      },\n";
        json << L"      \"direction\": {\n";
        json << L"        \"x\": " << light.direction.x << L",\n";
        json << L"        \"y\": " << light.direction.y << L",\n";
        json << L"        \"z\": " << light.direction.z << L"\n";
        json << L"      },\n";
        json << L"      \"intensity\": " << light.intensity << L",\n";
        json << L"      \"color\": {\n";
        json << L"        \"r\": " << (int)light.color.Red() << L",\n";
        json << L"        \"g\": " << (int)light.color.Green() << L",\n";
        json << L"        \"b\": " << (int)light.color.Blue() << L"\n";
        json << L"      }";

        if (light.isSpotLight)
        {
            json << L",\n";
            json << L"      \"spotLight\": {\n";
            json << L"        \"innerAngle\": " << light.innerAngle << L",\n";
            json << L"        \"outerAngle\": " << light.outerAngle << L"\n";
            json << L"      }";
        }

        json << L"\n    }";

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

std::string CLightEventWatcher::WStringToUTF8(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();

    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}