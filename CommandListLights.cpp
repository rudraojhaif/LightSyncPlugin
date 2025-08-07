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
#include "CommandListLights.h"
#include "LightUtils.h"

// Global static instance of the command - automatically registers with Rhino
static class CCommandListLights theListLightsCommand;

/**
 * @brief Returns the unique identifier for this command
 * @return UUID that uniquely identifies the ListLights command
 * @note This UUID should never change to maintain compatibility
 */
UUID CCommandListLights::CommandUUID()
{
    // Static UUID for ListLights command - generated once and remains constant
    static const GUID uuid = { 0x715FE371, 0xA00A, 0x4E37, {0xB2,0xAE,0x20,0x75,0x23,0xDF,0xFC,0xC9} };
    return uuid;
}

/**
 * @brief Returns the English name of the command as it appears in Rhino
 * @return Wide character string containing the command name
 * @note This is the name users will type in the command line
 */
const wchar_t* CCommandListLights::EnglishCommandName()
{
    return L"ListLights";
}

/**
 * @brief Main command execution method
 * @param context Command context containing document and other execution information
 * @return Command execution result (success, failure, etc.)
 *
 * This method:
 * 1. Validates the document context
 * 2. Retrieves all lights from the active document
 * 3. Prints a comprehensive inventory to the console
 * 4. Exports the light data to a file for backup/analysis
 */
CRhinoCommand::result CCommandListLights::RunCommand(const CRhinoCommandContext& context)
{
    // Validate document context - ensure we have a valid document to work with
    CRhinoDoc* doc = context.Document();
    if (nullptr == doc)
    {
        RhinoApp().Print(L"Error: No active document found.\n");
        return CRhinoCommand::failure;
    }

    try
    {
        // Get all lights using the utility class - this handles all light types
        std::vector<LightUtils::LightInfo> lights = LightUtils::GetAllLights(doc);

        // Print the light inventory to console - provides immediate feedback to user
        LightUtils::PrintLightInventory(lights);

        // Export data to file - creates a persistent record of the light configuration
        if (LightUtils::ExportLightsToFile(lights, LightUtils::DEFAULT_EXPORT_PATH))
        {
            // Inform user of successful export with file location
            RhinoApp().Print(L"Light data successfully exported to: %s\n", LightUtils::DEFAULT_EXPORT_PATH.c_str());
        }
        else
        {
            // Warn user if file export failed, but don't fail the entire command
            RhinoApp().Print(L"Warning: Failed to export light data to file.\n");
        }

        return CRhinoCommand::success;
    }
    catch (const std::exception& e)
    {
        // Handle standard C++ exceptions gracefully
        RhinoApp().Print(L"Error: An exception occurred during command execution.\n");
        return CRhinoCommand::failure;
    }
    catch (...)
    {
        // Catch-all for any other exception types
        RhinoApp().Print(L"Error: An unknown exception occurred during command execution.\n");
        return CRhinoCommand::failure;
    }
}