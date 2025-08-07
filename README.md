\# LightSyncPlugin Documentation



\## Overview



LightSyncPlugin is a Rhino plugin that exports light data to synchronize with Unreal Engine projects. It provides seamless integration between Rhino's lighting setup and UE4/UE5 environments.



\## Installation



1\. Setup your system for creating plugins for Rhino (C++) from official docs if you want to edit the source code: \[https://developer.rhino3d.com/guides/cpp/installing-tools-windows/](https://developer.rhino3d.com/guides/cpp/installing-tools-windows/)

2\. Load the LightSyncPlugin in Rhino (Drag and drop `/x64/Debug/LightSyncPlugin.rhp`)

3\. Ensure the plugin is properly registered and active



\## Usage



\### Step 1: Export Lights from Rhino



Run the following command in Rhino's command line:



```

ListLights

```



This command will:



\* Display all lights in the current scene with their properties

\* Export light data to `C:/ProgramData/RhinoLightSync/Lights.txt`



\### Step 2: Import Lights in Unreal Engine



1\. Open your Unreal Engine project

2\. Enter \*\*Play Mode\*\*

3\. Press \*\*`1`\*\* on your keyboard

4\. Click the \*\*"Sync Lights"\*\* button in the UI



The lights from Rhino will be automatically spawned in your Unreal Engine scene with matching properties.



\## Export File Format



The plugin exports lights in the following format:



```

\# RhinoLightSync Export File

\# Format: <Type> <Location> <Rotation> <Intensity> <Color> \[InnerAngle OuterAngle]



Point (x,y,z) (pitch°, yaw°, roll°) intensity RGB(r,g,b)

Directional (x,y,z) (pitch°, yaw°, roll°) intensity RGB(r,g,b)

Spot (x,y,z) (pitch°, yaw°, roll°) intensity RGB(r,g,b) innerAngle° outerAngle°

```



\### Example Output:



```

Spot (-19.7135,79.291,0) (-83.0946°, 0°, 0.00°) 1 RGB(212,0,0) 28.6479° 10.1624°

Point (0.785484,-4.24836,0) (0°, -90°, 0.00°) 3.6 RGB(255,188,0)

```



\## Supported Light Types



\* \*\*Point Lights\*\*: Omnidirectional lights with position and intensity

\* \*\*Directional Lights\*\*: Parallel rays simulating distant light sources

\* \*\*Spot Lights\*\*: Cone-shaped lights with inner/outer angles

\* \*\*Ambient Lights\*\*: Global illumination (exported but handled differently in UE) — \*Not yet implemented\*



\## Light Properties



\* \*\*Position\*\*: World coordinates (converted from Rhino to UE units)

\* \*\*Rotation\*\*: Pitch, Yaw, Roll in degrees

\* \*\*Intensity\*\*: Light strength (scaled appropriately for UE)

\* \*\*Color\*\*: RGB values (0-255 range)

\* \*\*Spot Angles\*\*: Inner and outer cone angles for spot lights



\## File Location



Light data is exported to:



```

C:/ProgramData/RhinoLightSync/Lights.txt

```



\## Troubleshooting



\* Ensure the export directory exists and is writable

\* Check Rhino's command line for any error messages during export

\* Verify that the Unreal Engine \*\*DSLightSyncer\*\* is properly configured

\* Make sure you're in \*\*Play Mode\*\* when syncing lights in Unreal



\## Future Versions



In future releases, we plan to introduce a \*\*proxy-based local sync mechanism\*\* that will eliminate the need for manual exporting from Rhino using the `ListLights` command and clicking the sync button in Unreal. Instead, lighting data will be \*\*live-synced automatically\*\* between Rhino and Unreal Engine during development.



\## Developer



\*\*Rudra Ojha\*\*

Email: \[rudraojhaif@gmail.com](mailto:rudraojhaif@gmail.com)



