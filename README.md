# LightSyncPlugin Documentation

## Overview

LightSyncPlugin is a Rhino plugin that provides real-time light synchronization with Unreal Engine projects through TCP communication. It offers seamless integration between Rhino's lighting setup and UE4/UE5 environments with automatic live updates when lights are modified, added, or deleted in Rhino.

While Datasmith provides light syncing by default, this plugin offers **much more stable and reliable light synchronization** specifically optimized for Rhino workflows.

## Features

- **Real-time TCP Communication**: Live synchronization between Rhino and Unreal Engine
- **Automatic Event Handling**: Responds to light additions, deletions, modifications, and undeletions
- **Smart Blacklist Management**: Tracks deleted lights to prevent ghost lights in Unreal
- **Unit Conversion**: Automatically converts Rhino units to meters (Unreal's standard)
- **Comprehensive Light Support**: Point, Directional, and Spot lights with full property mapping
- **Background Processing**: Non-blocking TCP communication to maintain UI responsiveness

## Installation

### Rhino Plugin Setup

1. Setup your system for creating plugins for Rhino (C++) from official docs if you want to edit the source code: https://developer.rhino3d.com/guides/cpp/installing-tools-windows/
2. Load the LightSyncPlugin in Rhino (Drag and drop `/x64/Debug/LightSyncPlugin.rhp`)
3. Ensure the plugin is properly registered and active

### Unreal Engine Setup

1. Clone or download the Unreal Engine project: https://github.com/rudraojhaif/DatasmithTest
2. Open the project in Unreal Engine
3. The project includes the TCP listener that receives light data from Rhino

## Usage

### Automatic Real-Time Synchronization

Once both Rhino plugin and Unreal project are running:

1. **Start Play Mode** in Unreal Engine
2. Press **`1`** on your keyboard to open the light sync UI
3. Click **"Sync Lights"** button to establish connection
4. **That's it!** Any changes to lights in Rhino will now automatically update in Unreal Engine in real-time

### Supported Operations

- **Add Light**: New lights appear instantly in Unreal
- **Delete Light**: Lights are removed from Unreal and blacklisted
- **Modify Light**: Position, rotation, intensity, and color changes sync immediately  
- **Undelete Light**: Restored lights are removed from blacklist and reappear in Unreal

### Manual Export (Legacy/Backup)

You can still manually export lights using the command:

```
ListLights
```

This exports light data to `C:/ProgramData/RhinoLightSync/Lights.txt` as a backup.

## Technical Implementation

### TCP Communication

The plugin uses TCP sockets for reliable communication:

- **Default Port**: 5173
- **Protocol**: JSON over TCP
- **Connection**: localhost (127.0.0.1)
- **Timeout**: 5 seconds
- **Threading**: Asynchronous to prevent UI blocking

### Light Event Handling

The `CLightEventWatcher` class monitors Rhino's light table events:

```cpp
void LightTableEvent(CRhinoEventWatcher::light_event event,
    const CRhinoLightTable& table, int lightIndex, const ON_Light* light)
```

### JSON Data Format

Light data is sent as structured JSON:

```json
{
  "event": "Light Modified",
  "lightCount": 2,
  "lights": [
    {
      "id": 0,
      "type": "Spot",
      "location": {"x": -19.713500, "y": 79.291000, "z": 0.000000},
      "rotation": {"pitch": -83.095, "yaw": 0.000, "roll": 0.000},
      "intensity": 1,
      "color": {"r": 212, "g": 0, "b": 0},
      "spotLight": {"innerAngle": 28.648, "outerAngle": 10.162}
    }
  ]
}
```

### Unit Conversion

The plugin automatically handles unit conversion from Rhino's model units to meters:

- Millimeters: × 0.001
- Centimeters: × 0.01  
- Inches: × 0.0254
- Feet: × 0.3048
- And more...

## Supported Light Types

- **Point Lights**: Omnidirectional lights with position and intensity
- **Directional Lights**: Parallel rays simulating distant light sources  
- **Spot Lights**: Cone-shaped lights with inner/outer angles
- **Ambient Lights**: Global illumination *(planned for future release)*

## Light Properties

- **Position**: World coordinates (automatically converted to meters)
- **Rotation**: Pitch, Yaw, Roll in degrees
- **Intensity**: Light strength
- **Color**: RGB values (0-255 range)
- **Spot Angles**: Inner and outer cone angles for spot lights

## File Locations

- **Export Path**: `C:/ProgramData/RhinoLightSync/Lights.txt`
- **Unreal Project**: https://github.com/rudraojhaif/DatasmithTest

## Advantages Over Datasmith

While Datasmith provides built-in light synchronization, this plugin offers:

- **Enhanced Stability**: More reliable light syncing specifically optimized for Rhino
- **Real-time Updates**: Immediate synchronization without manual refresh
- **Better Error Handling**: Robust TCP communication with timeout protection
- **Smart State Management**: Blacklist system prevents deleted light artifacts
- **Optimized Performance**: Background processing maintains UI responsiveness

## Troubleshooting

- **Connection Issues**: Ensure both Rhino plugin and Unreal project are running
- **Port Conflicts**: Check if port 5173 is available
- **Missing Updates**: Verify you're in Play Mode in Unreal Engine  
- **File Permissions**: Ensure write access to `C:/ProgramData/RhinoLightSync/`
- **Unit Scaling**: Check that your Rhino model units are properly set

## System Requirements

- **Rhino**: 6.0 or later
- **Unreal Engine**: 4.27+ or UE5
- **Operating System**: Windows (Winsock2 required)
- **Network**: TCP/IP stack enabled

## Future Development

- Ambient light support
- Advanced light properties (shadows, falloff curves)
- Multi-document synchronization
- Custom port configuration
- Light grouping and batch operations

## Developer

**Rudra Ojha**  
Email: [rudraojhaif@gmail.com](mailto:rudraojhaif@gmail.com)

## License

Copyright (c) 2025 Rudra Ojha. All rights reserved.

This source code is provided for educational and reference purposes only. Redistribution, modification, or use of this code in any commercial or private product is strictly prohibited without explicit written permission from the author.