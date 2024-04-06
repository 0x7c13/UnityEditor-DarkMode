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

  > **WARNING:** if you feel uncomfortable downloading a malicious DLL from a stranger like me from the internet, then you should not:) Take a look at the last section to see how it works and how to build it yourself if you wish. Please do your own homework and make your own judgement.
- Add a Unity editor script to your project like below:
    ```C#
    namespace Editor.Theme // Change this to your own namespace you like or simply remove it
    {
        using System.Runtime.InteropServices;
        using UnityEditor;

        public static class DarkMode
        {
            [InitializeOnLoadMethod]
            // Change below path to the path of the downloaded dll or
            // simply put the dll in the same directory as the script and use
            // [DllImport("UnityEditorDarkMode.dll", EntryPoint = "DllMain")] instead
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

Ok, so what I have done on top of `ReaperThemeHackDll` is:
- Remove all the unnecessary dependencies of Reaper framework and plugin code since we don't need them for Unity Editor hack.
- Instead of checking the window class name of `REAPERwnd` when doing the sub-classing using `SetWindowSubclass` API, I use `UnityContainerWndClass` instead, which is the window class name of Unity Editor main window. You can get this window class name by using below Win32 API:
    ```C#
    [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    private static extern int GetClassName(IntPtr hWnd, char[] lpClassName, int nMaxCount);
    ```
- I have verified that the window class name is consistent across different Unity Editor versions (2020, 2021, 2022, 2023, Unity 6) so it should work on most versions. If not, you should probably make further modifications to the code to use the C++ `GetClassName` Win32 API to get the window class name dynamically at runtime.
  > **NOTE:** If you do this, it basically means this hack can be used for any Windows application that uses the default white Win32 title bar, menu bar, context menu, etc.
- A different color preset is given by default which I think looks better with Unity Editor.
- Some inrelevent code is also removed and some minor modifications are made to make it more performant and clean. You don't have to do it tho so I am not going to explain them here.
- The built dll is a standard Win32 DLL with a DllMain entry point, meaning it can be side-loaded using Detours `withdll.exe`. This has been explained in the ReaperThemeHackDll repository as well. However, one caveat is that if you run below command:
    ```cmd
    .\withdll.exe /d:UnityEditorDarkMode.dll ^
    "C:\Program Files\Unity\Hub\Editor\2022.3.22f1\Editor\Unity.exe"
    ```
    It will NOT work because invoking Unity.exe directly will launch the Unity Hub first. After you open the project from Unity Hub, it will launch the Unity Editor. But the context is getting lost from here. So instead of running above command, you should always give your project path as an argument to Unity.exe like below if you wish to use `withdll.exe` instead of the editor script approach:
    ```cmd
    .\withdll.exe /d:UnityEditorDarkMode.dll ^
    "C:\Program Files\Unity\Hub\Editor\2022.3.22f1\Editor\Unity.exe" ^
    -projectPath "C:\<Path>\<To>\<Your>\<UnityProjectFolder>"
    ```

## Known issues
> I haven't found any issues so far using it with Unity 2021, 2022, 2023, and Unity 6 on Windows 11.

However, I do observe an issue with `Unity 2020`. It looks like the `[InitializeOnLoadMethod]` is getting called before the main window is created in Unity 2020. This is causing the sub-classing to fail. It looks like that [InitializeOnLoadMethod] actually getting called when the splash window is shown on Unity 2020. So a workaround is to manually call the `_()` method after the main window is created by sleep for some time. You can do this by making below changes to the editor script:
```C#
namespace Editor.Theme // Change this to your own namespace you like or simply remove it
{
    using System.Runtime.InteropServices;
    using System.Threading;
    using UnityEditor;

    public static class DarkMode
    {
        // Change below path to the path of the downloaded dll or
        // simply put the dll in the same directory as the script and use
        // [DllImport("UnityEditorDarkMode.dll", EntryPoint = "DllMain")] instead
        [DllImport(@"C:\Users\<...>\Desktop\UnityEditorDarkMode.dll", EntryPoint = "DllMain")]
        private static extern void _();

        [InitializeOnLoadMethod]
        public static void EnableDarkMode()
        {
            Thread.Sleep(100); // Sleep for 100ms to wait for the main window to be created
            _();
        }
    }
}
```