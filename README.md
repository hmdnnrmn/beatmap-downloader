# osu! Beatmap Downloader

An automatic beatmap downloader for the osu! rhythm game.

## Features

*   **Automatic Downloads:** Automatically downloads beatmap files when a beatmap link is copied to the clipboard or opened in a web browser.
*   **In-Game Overlay:** Displays download progress and status notifications directly within the osu! game client.
*   **Seamless Integration:** Works by injecting a DLL into the osu! process, allowing for a smooth and uninterrupted gameplay experience.
*   **Customizable Configuration:** Settings can be adjusted through a configuration file, giving users control over the downloader's behavior.

## How It Works

The osu! Beatmap Downloader is implemented as a DLL that is injected into the osu! game process. Once injected, it uses a combination of techniques to monitor for beatmap download links:

*   **Shell Hooking:** The downloader hooks the `ShellExecuteExW` function to intercept URLs that are opened by the game. When a beatmap URL is detected, the downloader initiates the download process.
*   **Clipboard Monitoring:** The downloader also monitors the system clipboard for beatmap links. When a link is copied, it is added to the download queue.
*   **Download Manager:** A dedicated download manager handles the downloading of beatmap files in the background. It provides progress updates and ensures that downloads are completed successfully.
*   **In-Game Overlay:** An OpenGL-based overlay is used to display download notifications and progress bars directly within the game, allowing players to stay informed without leaving the client.

## Building from Source

To build the osu! Beatmap Downloader from source, you will need:

*   A C++ compiler that supports C++17
*   The Windows SDK
*   Microsoft Visual Studio

Once you have the necessary tools, you can open the `osu_beatmap_downloader.vcxproj` file in Visual Studio and build the solution. The output will be a DLL file that can be injected into the osu! game client.
