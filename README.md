# DarkMode Mod for UnityEditor on Windows
 <a style="text-decoration:none" href="https://github.com/0x7c13/UnityEditor-DarkMode/actions/workflows/ci.yml"><img src="https://img.shields.io/github/actions/workflow/status/0x7c13/UnityEditor-DarkMode/ci.yml" alt="Size" /></a> 

A fully working runtime dark mode mod for Unity Editor on Windows with:
- Dark title bar
- Dark menu bar
- Dark context menu
- And more...

> This runtime mod works on Windows 11 and Windows 10 1903+. Tested on Unity 2019, 2020, 2021, 2022, 2023 and Unity 6.

![Screenshot](screenshot.png?raw=true)

## How to use it?
- Download the `UnityEditorDarkMode.dll` from [releases](https://github.com/0x7c13/UnityEditor-DarkMode/releases)

  > **WARNING:** If you feel uncomfortable downloading a malicious DLL from a stranger like me, then you should not:) Take a look at later sections to see how it works and how to build it yourself if you prefer. Please do your own homework and make your own judgement. I offer this approach as a convenience for those who don't want to build a C++ project themselves.
- Now, you have two options:

    Option 1: Add a Unity editor script to your project like below:
    ```C#
    #if UNITY_EDITOR_WIN // Windows only, obviously
    namespace Editor.Theme // Change this to your own namespace you like or simply remove it
    {
        using System.Runtime.InteropServices;
        using UnityEditor;

        public static class UnityEditorDarkMode
        {
            // Change below path to the path of the downloaded dll or
            // simply put the dll in the same directory as the script and use
            // [DllImport("UnityEditorDarkMode.dll", EntryPoint = "DllMain")] instead
            [DllImport(@"C:\Users\<...>\Desktop\UnityEditorDarkMode.dll", EntryPoint = "DllMain")]
            private static extern void _();

            [InitializeOnLoadMethod]
            private static void __()
            {
                #if !UNITY_2021_1_OR_NEWER
                // [InitializeOnLoadMethod] is getting called before the editor
                // main window is created on earlier versions of Unity, so we
                // need to wait a bit here before attaching the dll.
                System.Threading.Thread.Sleep(100);
                #endif
                _(); // Attach the dll to the Unity Editor
            }
        }
    }
    #endif
    ```
    Option 2 (Unity 2021 or higher): Copy the DLL into your Unity project and apply below settings to the DLL in the Unity Editor inspector:

    ![dll-setting](screenshot-dll-setting.png?raw=true)
    
    Make sure `Load on startup` is checked which will make the DLL to be loaded on Unity Editor startup.

    > **NOTE:** You could also inject the dll using `withdll.exe` from [Detours](https://github.com/microsoft/Detours). Instructions are provided in the later sections for the use of `withdll.exe`.
- Restart Unity Editor and you are done!
- Now enjoy the immersive dark mode in Unity Editor!

## How to change the theme?
After first launch, a `UnityEditorDarkMode.dll.ini` file will be created in the same directory as the dll. You can modify the values in this file to change the theme (Restart the editor after changing the values).

## How to remove it?
Remove the Unity editor script you added to your project and that's it! All magic happens at runtime:)

## How to build it?
- Make sure latest `CMake`, `Visual Studio` and `MSVC toolchain` are installed on your system. Then run below command in the project directory:

    ```cmd
    cmake -B build && cmake --build build --config Release
    ```
    > NOTE: You may need to add `cmake` to your system path if you haven't already.

- The `UnityEditorDarkMode.dll` will be created under the `build\Release` directory after the build finishes successfully.

## How it works?
This project is basically a stripped down version of [ReaperThemeHackDll](https://github.com/jjYBdx4IL/ReaperThemeHackDll) made by [jjYBdx4IL](https://github.com/jjYBdx4IL) with some minor modifications. If you like this project, please consider giving a star to the `ReaperThemeHackDll` project as well. Actually, his code with some minor modifications can be used to theme any legacy Windows applications that uses the Win32 title bar, menu bar, context menu, etc.

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
> I haven't found any major issues so far. Please let me know if you find any issues by creating an issue in this repository. 

The reason why the DLL injection script is different for `Unity 2021 or later` and `Unity 2020 or earlier` is because the `[InitializeOnLoadMethod]` is getting called before the main window is created in earlier versions of Unity (2019, 2020). This is causing the sub-classing to fail since the main window is not found at dll attach time. So a simple workaround is to manually invoke the dll after the main window is created by sleeping the thread for some time. 100 ms is about right but you can adjust it if you encounter issues.
