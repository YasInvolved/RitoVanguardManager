# RitoVanguardManager
Not really a simple program for enabling and disabling vanguard automatically so you don't need to worry about potential spyies on your computer.

## How does it work
On program startup (requires administrator) it enables riot vanguard service, then it turns on League of Legends and wait until League Client process shuts off. 
When it does, the program disables riot vanguard end quits.

## TODO
- Comments in code (it's hardly readable)
- Valorant and other riot games support
- Better config (for now it's a single line file with path to riot client)
- Configurator (automatically search for riot client etc.)
- Maybe optimization

## Contributions
Any contributions are always welcome. I think some people may consider this software as useful, so if you know C++ and you can write WinAPI code you can add something.
