// LightSyncPluginPlugIn.cpp : defines the initialization routines for the plug-in.
//

#include "stdafx.h"
#include "rhinoSdkPlugInDeclare.h"
#include "LightSyncPluginPlugIn.h"
#include "Resource.h"
#include "LightEventWatcher.h"

// The plug-in object must be constructed before any plug-in classes derived
// from CRhinoCommand. The #pragma init_seg(lib) ensures that this happens.
#pragma warning(push)
#pragma warning(disable : 4073)
#pragma init_seg(lib)
#pragma warning(pop)

// Rhino plug-in declaration
RHINO_PLUG_IN_DECLARE

// Rhino plug-in name
RHINO_PLUG_IN_NAME(L"LightSyncPlugin");

// Rhino plug-in id
RHINO_PLUG_IN_ID(L"5604EC7E-D0D2-4A9D-8B7C-D9486550BE38");

// Rhino plug-in version
RHINO_PLUG_IN_VERSION(__DATE__ "1.0.0" __TIME__);

// Rhino plug-in description
RHINO_PLUG_IN_DESCRIPTION(L"Synchronize lights between Rhino and external applications");

// Rhino plug-in icon resource id
RHINO_PLUG_IN_ICON_RESOURCE_ID(IDI_ICON);

// Rhino plug-in developer declarations
RHINO_PLUG_IN_DEVELOPER_ORGANIZATION(L"Rudra Ojha");
RHINO_PLUG_IN_DEVELOPER_ADDRESS(L"India");
RHINO_PLUG_IN_DEVELOPER_COUNTRY(L"India");
RHINO_PLUG_IN_DEVELOPER_PHONE(L"");
RHINO_PLUG_IN_DEVELOPER_FAX(L"");
RHINO_PLUG_IN_DEVELOPER_EMAIL(L"rudraojhaif@gmail.com");
RHINO_PLUG_IN_DEVELOPER_WEBSITE(L"");
RHINO_PLUG_IN_UPDATE_URL(L"");

// The one and only CLightSyncPluginPlugIn object
static class CLightSyncPluginPlugIn thePlugIn;

// The only event watchers
static class CLightEventWatcher g_LightEventWatcher;
/////////////////////////////////////////////////////////////////////////////
// CLightSyncPluginPlugIn definition

CLightSyncPluginPlugIn& LightSyncPluginPlugIn()
{
	// Return a reference to the one and only CLightSyncPluginPlugIn object
	return thePlugIn;
}

CLightSyncPluginPlugIn::CLightSyncPluginPlugIn()
{
	// CLightSyncPluginPlugIn constructor. The constructor is called when the
	// plug-in is loaded and "thePlugIn" is constructed. Once the plug-in
	// is loaded, CLightSyncPluginPlugIn::OnLoadPlugIn() is called.
	m_plugin_version = RhinoPlugInVersion();
}

/////////////////////////////////////////////////////////////////////////////
// Required overrides

const wchar_t* CLightSyncPluginPlugIn::PlugInName() const
{
	// Plug-in name display string shown in Rhino when loading the plug-in
	// and in the plug-in management interface
	return RhinoPlugInName();
}

const wchar_t* CLightSyncPluginPlugIn::PlugInVersion() const
{
	// Plug-in version display string shown by Rhino when loading the plug-in
	// and in the plug-in management interface
	return m_plugin_version;
}

GUID CLightSyncPluginPlugIn::PlugInID() const
{
	// Plug-in unique identifier used by Rhino to manage the plug-ins
	return ON_UuidFromString(RhinoPlugInId());
}

/////////////////////////////////////////////////////////////////////////////
// Additional overrides

BOOL CLightSyncPluginPlugIn::OnLoadPlugIn()
{
	// Called after the plug-in is loaded and the constructor has been run.
	// This is where significant initialization should be performed.
	// This function must return TRUE for the plug-in to continue loading.
	// Turn on event watcher
	g_LightEventWatcher.Register();
	g_LightEventWatcher.Enable(TRUE);
	// Initialize the light sync system
	return TRUE;
}

void CLightSyncPluginPlugIn::OnUnloadPlugIn()
{
	// Called when plug-in is about to be unloaded.
	// Only manipulate your own objects here as Rhino's systems may be shutting down.
	// Turn off event watcher
	g_LightEventWatcher.Enable(FALSE);
	// Clean up any resources used by the light sync system
}

