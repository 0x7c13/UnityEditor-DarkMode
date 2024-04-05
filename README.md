# UnityEditor DarkMode for Windows 11
A fully working dark mode mod for Unity Editor on Windows 11 with:
- Dark title bar
- Dark menu bar
- Dark context menu
- And more...

![Screenshot](screenshot.png?raw=true)

## How to use it?
- Download the [UnityEditorDarkMode.dll](UnityEditorDarkMode.dll) to your Windows 11 PC.
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
            public static extern void DllMain();
        }
    }
    ```
- Restart Unity Editor if needed.
- Enjoy the dark mode!

## How to change the theme?
After first launch, a `UnityEditorDarkMode.dll.ini` file will be created in the same directory as the dll. You can modify the values in this file to change the theme.

## How it works?
This project is basically a special build of [ReaperThemeHackDll](https://github.com/jjYBdx4IL/ReaperThemeHackDll) made by [jjYBdx4IL](https://github.com/jjYBdx4IL) with some minor modifications. Feel free to take a look at the source code of ReaperThemeHackDll if you are intersted.
