#pragma once

#include "stdafx.h"
#include "rhinoSdkCommand.h"
#include <fstream>
#include <sstream>

/**
 * Rhino command to list all lights in the scene and export them to a text file.
 * Enumerates all lights in the current Rhino document, displays their properties
 * in the command line, and exports the data to a structured text file.
 */
class CCommandListLights : public CRhinoCommand
{
public:
    CCommandListLights() = default;

    UUID CommandUUID() override;
    const wchar_t* EnglishCommandName() override;
    CRhinoCommand::result RunCommand(const CRhinoCommandContext& context) override;
};