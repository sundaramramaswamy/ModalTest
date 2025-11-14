# ModalTest - Windows App SDK Bug Reproduction

This is a minimal C++/WinRT application that has hello world win32 app and modal app using desktopChildSitebridge.


## Reproduction Steps For crash with Modal dangling ref issue

1. Build and run `ModalTest.cpp`
2. Click "Show Modal" button
3. Click the "X" button on the modal to close it
4. **Click anywhere on the parent window**
5. **Application crashes** with FailFast in `contentappwindowbridge.cpp(835)`


### Architecture
- **Parent Window**: Win32 HWND with `DesktopChildSiteBridge` connected to `ContentIsland`
- **Modal Window**: `DesktopPopupSiteBridge` with parent island as parent
- **Cleanup Sequence**: Hide → Close popup → Close island → nullptr

### Key Files
- `ModalTest.cpp` - Main test application demonstrating the bug
- `DesktopAppWin32.vcxproj` - Visual Studio project file

## Build Requirements

- Visual Studio 2022 or later
- Windows App SDK 1.7 or later
- C++/WinRT support
- Windows 10 SDK (10.0.19041.0 or later)

## Expected vs Actual Behavior

### Expected
Modal closes cleanly, parent window remains fully functional.

### Actual
Modal closes successfully, but clicking parent window crashes with:
```
contentappwindowbridge.cpp(835)\Microsoft.UI.Input.dll!00007FF8BC10BFD5: 
FailFast(1) tid(9e90) 8000FFFF Catastrophic failure
```
