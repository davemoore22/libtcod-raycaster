**Libtcod-Raycaster**

*A simple libtcod/SDL2 raycaster written in C++17 and released under the MIT License*

The purpose of this code snippet to display a dungeon from a first-person perspective. It's an adaptation of the SDL2 raycaster written by Timmos (<https://github.com/T1mmos/raycaster-sdl>) quickly grafted onto a libtcod (<https://github.com/libtcod/libtcod/>) front end. The graphics tiles used are the same Wolfenstein 3D textures.

Note this is ludicrously slow compared to Timmos's original as I'm doing a (probably) suboptimal reading of the SDL surface output of the Raycaster and copying it pixel by pixel onto a libtcod buffer for display. Contributions welcome to speed this up.

Included is a Codeblocks project usable under Ubuntu Linux. It uses libtcod 1.11.1 (but I think it works as far back as libtcod 1.6).

To compile in other editors/IDEs include libtcod, SDL2, and SDL2_image libraries.

Comments and criticisms and more info e-mail me at davemoore22@gmail.com

