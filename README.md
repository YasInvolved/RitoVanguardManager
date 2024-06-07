# RitoVanguardManager (yeah "Rito" is on purpose 😉)
Not really a simple program for enabling and disabling vanguard automatically so you don't need to worry about potential spyies on your computer.

## How does it work
On program startup (requires administrator) it enables riot vanguard service, then it turns on League of Legends and wait until League Client process shuts off. 
When it does, the program disables riot vanguard end quits.

## How to use it
1. Unpack zip archieve with executable and config file (VanguardManager.conf)
2. if your riot client path differs from `C:\Riot Games\Riot Client\RiotClientServices.exe` then change it to match yours. It's a single-line file so you don't need to worry about complications.
3. (optional) create shortcut to application and place it in the desktop
4. just run the app

## TODO
- Comments in code (it's hardly readable)
- Automatic builds (GitHub workflow)
- Valorant and other riot games support
- Better config (for now it's a single line file with path to riot client)
- Configurator (automatically search for riot client etc.)
- Maybe optimization

## Contributions
Any contributions are always welcome. I think some people may consider this software as useful, so if you know C++ and you can write WinAPI code you can add something.

## Current state
Well, for now I won't be contributing to the project since I have other projects to advance in. Ofc if I find the time and need to work on this I'll do that.
