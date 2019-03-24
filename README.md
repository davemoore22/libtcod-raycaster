**Libtcod-Raycaster**

*A simple libtcod/SDL2 raycaster written in C++17 and released under the MIT License*

The purpose of this code snippet to display a dungeon from a first-person perspective using the libtcod (<https://github.com/libtcod/libtcod/>) truecolour console most often used for Roguelike development. It's an adaptation of the SDL2 raycaster written by Timmos (<https://github.com/T1mmos/raycaster-sdl>) quickly grafted onto libtcod. The graphics tiles used are the same Wolfenstein 3D textures as provided by Timmos's original.

Note this is ludicrously slow compared to Timmos's original as I'm doing an extremely suboptimal reading of the SDL surface output of the Raycaster and copying it pixel by pixel onto a libtcod buffer for display. Contributions welcome to speed this up as I don't know much SDL.

Included is a Codeblocks project usable under Ubuntu Linux. It uses libtcod 1.11.1 (but I think it works as far back as libtcod 1.6).

To compile in other editors/IDEs include libtcod, SDL2, and SDL2_image libraries.

Comments and criticisms and more info e-mail me at davemoore22@gmail.com

![Unoptimised Example](https://media.giphy.com/media/iMCeomYyKH1Lb7njYU/giphy.gif)

