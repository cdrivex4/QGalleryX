# ReShade Quick User Guide (UT2004)

This guide explains how to control and configure ReShade while playing Unreal Tournament 2004.

## Core Controls
| Action | Key |
| :--- | :--- |
| **Toggle Menu (Overlay)** | `Home` |
| **Toggle All Effects (On/Off)** | `Scroll Lock` |
| **Take Screenshot** | `Print Screen` (Default) |

## Basic Usage
1.  **Launch the Game**: Always use `Play_ReShade.bat` from your game folder to ensure ReShade loads.
2.  **Enable Effects**: 
    - Press `Home` to open the ReShade menu.
    - Check the boxes next to the shaders you want to enable (e.g., Bloom, Sharpen).
3.  **Adjust Settings**: If you select a shader, you can fine-tune its settings in the bottom pane of the `Home` tab.
4.  **Save Changes**: ReShade saves your current configuration to `DefaultPreset.ini` automatically when you close the menu.

## Troubleshooting
- **Effects not showing?** Ensure "Performance Mode" is unchecked at the bottom of the Home tab if you want to edit settings.
- **Menu won't open?** Check that you launched via the `.bat` file and that `d3d9.dll` exists in the `System` folder.
