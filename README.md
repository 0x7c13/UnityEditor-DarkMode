# DarkMode Mod for UnityEditor on Windows
A fully working dark mode mod for Unity Editor on Windows with:
- Dark title bar
- Dark menu bar
- Dark context menu
- And more...

This works 100% on Windows 11 and should work on Windows 10 as well (most likely, not tested).

![Screenshot](screenshot.png?raw=true)

## How to use it?
- Download the `UnityEditorDarkMode.dll` from [releases](https://github.com/0x7c13/UnityEditor-DarkMode/releases)
- Add a Unity editor script to your project like below:
    ```C#
    namespace Editor
    {
        using System.Runtime.InteropServices;
        using UnityEditor;

        public static class DarkMode
        {
            [InitializeOnLoadMethod]
            // Change below path to the path of the downloaded dll
            [DllImport(@"C:\Users\<...>\Desktop\UnityEditorDarkMode.dll", EntryPoint = "DllMain")]
            public static extern void _();
        }
    }
    ```
- Restart Unity Editor and you are done!
- Now enjoy the immersive dark mode in Unity Editor!

## How to change the theme?
After first launch, a `UnityEditorDarkMode.dll.ini` file will be created in the same directory as the dll. You can modify the values in this file to change the theme (Restart the editor after changing the values).

## How to remove it?
Remove the Unity editor script you added to your project and that's it! All magic happens at runtime:)

## How it works?
This project is basically a special build of [ReaperThemeHackDll](https://github.com/jjYBdx4IL/ReaperThemeHackDll) made by [jjYBdx4IL](https://github.com/jjYBdx4IL) with some minor modifications (That's why I did not include my source code of the dll in this repository since I don't want to take his credit). Feel free to take a look at the source code of ReaperThemeHackDll if you are intersted. If you like this project, please consider giving a star to the ReaperThemeHackDll project as well. Actually, his code with some minor modifications can be used to theme any legacy Windows applications that uses the Win32 title bar, menu bar, context menu, etc.

