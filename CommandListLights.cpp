#include "stdafx.h"
#include "CommandListLights.h"
#include "LightUtils.h"

// Global static instance of the command
static class CCommandListLights theListLightsCommand;

UUID CCommandListLights::CommandUUID()
{
    // Static UUID for ListLights command
    static const GUID uuid = { 0x715FE371, 0xA00A, 0x4E37, {0xB2,0xAE,0x20,0x75,0x23,0xDF,0xFC,0xC9} };
    return uuid;
}

const wchar_t* CCommandListLights::EnglishCommandName()
{
    return L"ListLights";
}

CRhinoCommand::result CCommandListLights::RunCommand(const CRhinoCommandContext& context)
{
    // Validate document context
    CRhinoDoc* doc = context.Document();
    if (nullptr == doc)
    {
        RhinoApp().Print(L"Error: No active document found.\n");
        return CRhinoCommand::failure;
    }

    try
    {
        // Get all lights using the utility class
        std::vector<LightUtils::LightInfo> lights = LightUtils::GetAllLights(doc);

        // Print the light inventory to console
        LightUtils::PrintLightInventory(lights);

        // Export data to file
        if (LightUtils::ExportLightsToFile(lights, LightUtils::DEFAULT_EXPORT_PATH))
        {
            RhinoApp().Print(L"Light data successfully exported to: %s\n", LightUtils::DEFAULT_EXPORT_PATH.c_str());
        }
        else
        {
            RhinoApp().Print(L"Warning: Failed to export light data to file.\n");
        }

        return CRhinoCommand::success;
    }
    catch (const std::exception& e)
    {
        RhinoApp().Print(L"Error: An exception occurred during command execution.\n");
        return CRhinoCommand::failure;
    }
    catch (...)
    {
        RhinoApp().Print(L"Error: An unknown exception occurred during command execution.\n");
        return CRhinoCommand::failure;
    }
}