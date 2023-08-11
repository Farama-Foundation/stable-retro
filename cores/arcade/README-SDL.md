# FinalBurn Neo for SDL1.2/2 

## Differences between SDL1.2 and SDL2 versions

The SDL2 port is the recommended version as it has the following features over the SDL1.2 port:

* A better renderer
* OSD  - shows when fast-forwarding and when FPS display is enabled via F12
* Game selection menu which also replicates the rom scanning functionality of the Windows version of FBNeo. Handy for cabinets!
* A lot more testing

## Compiling
### SDL1.2

Assuming you have a working GCC (Mingw and GCC under ubuntu 18.04 have been tested) you can just install libsdl 1.2 and type:

`make sdl`

And out will pop an fbneo executable.

### SDL2

Assuming you have a working GCC (Mingw and GCC under ubuntu 18.04 have been tested) you can just install libsdl 2 and libsdl2-image and type:

'make sdl2'

and out will pop an fbneo executable.

### SDL2 + CRT mode switching

Linux now supports modeswitching for CRT monitors using https://github.com/antonioginer/switchres (which is the switchres code of GroovyMAME extracted by its author as a library for emulators that wish to implement resolution switching). In order to get this to work, you need to:
  * `git clone https://github.com/antonioginer/switchres.git` + `make` then `make install`. You'll also need to copy the repos `switchres.ini` to `/etc` and customize it. Monitors presets can be found at http://forum.arcadecontrols.com/index.php/topic,116023.msg1230485.html#msg1230485
  * build fbneo with `make INCLUDE_SWITCHRES=1 sdl2`

Current known limitations:
  * the SDL2 backend doesn't support rom interlace. Although switchres can compute an interlace resolution, FBNeo isn't yet passing that flag.
  * Vertical screens are not detected, so vertical games will always be rendered as horizontal
  * in-game resolution switching is not yet supported
  * if the monitor's refresh rate is too different from the game refresh rate, no mechanism comes to smooth the video output, so tearing can happen.

## Running

### SDL1.2

* First run

run the fbneo executable and an fbneo.ini will be created. You will need to edit this to point to your rom directories. You can also edit any of the other otions in the ini file as needed

* subsiquent runs

type 

'fbneo <romname>' 

where <romname> is the name of a supported rom. For example

'fbneo sf2'

will run sf2. 

### SDL2

'-cd' used when running a neocd game. You'll have to look in the code to work out where the the CD images should be placed

'-joy' enable joystick

'-menu' load the menu

'-novsync' disable vsync

'-integerscale' only scale to the closest integer value. This will cause a border around the games unless you are running at a resolution that is a whole multiple of the games original resolution

'-fullscreen' enable fullscreen mode

'-dat' generate dat files

'-autosave' autosave/autoload a save state when starting or exiting a game

'-nearest' enbable nearest neighbour filtering (e.g. just scale the pixels)

'-linear' enable linear filter (or is it a bilinear filter) to smooth out pixels

'-best' enable sdl2 'best' filtering, which actually makes the games look the worst
 

recommend command line options:

'fbneo -menu -integerscale -fullscreen -joy'

The above will give you a nicely scalend game screen and the menu for launching games. 

## In-game controls

'TAB' - brings up the in game menu (or game PAUSE)
'F12' - quit game.
'F1' - fast forward game.
'F11' - show FPS counter
'F6' - Screenshot
'ALT-ENTER' - Switch window/fullscreen
'+'/'-' - Volume controls

## SDL2 in menu controls

'F1' - Rescan current roms
'F2' - enable/disable filtering
'F3' - Swap current system
'F12' - quit menu. This will return you to the game select menu if run with '-menu'. Press 'f12' again to quit 
'q'/'w' - Skip to next letter
'ALT-ENTER' - Switch window/fullscreen