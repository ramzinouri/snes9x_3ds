# ![Snes9x for 3DS](https://github.com/ramzinouri/snes9x_3ds/blob/master/assets/logo.png?raw=1)
Snes9x for 3DS is a high-compatibility SNES emulator for your Old 3DS / 2DS. It runs many games at full speed (60 fps). It supports games that use CX4 chip (Megaman X2/X3), and the SDD-1 chip (Star Ocean, Super Street Fighter 2 Alpha). It can also play games that use the SuperFX chip (Yoshi's Island, etc), and the SA-1 chip (Super Mario RPG, Kirby's Dreamland, Kirby Super Star) but it runs slowly. It has generally much high compatibility than existing SNES emulators on the old 3DS because it uses Snes9x 1.43 as a base, and is a good alternative if your existing emulators cannot play all the games.

It also runs on your New 3DS, and it runs games faster! Use your New 3DS if you want to play SuperFX and SA-1 games!


## How to use:

### Homebrew Launcher:

1. Copy `Snes9x_3ds.3dsx`, `Snes9x_3ds.smdh` into  `sd:/3ds/snes9x_3ds`.
2. Place your SNES ROMs inside any folder.
3. Go to your Homebrew Launcher (either via Cubic Ninja, Soundhax or other entry points) and launch the snes9x_3ds emulator.

### CIA Version:

1. Use your favorite CIA installer to install `snes9x_3ds.cia` into your CFW. You can also use FBI to download from TitleDB.
2. Place your SNES ROMs inside any folder.
3. Exit your CIA installer and go to your CFW's home screen to launch the app.

### When in-game,

1. Tap the bottom screen for the menu.
2. Use Up/Down to choose option, and A to confirm.
   (Hold down X while pressing Up/Down to Page Up/Page Down)
3. Use Left/Right to change between ROM selection and emulator options.
4. You can quit the emulator to your homebrew launcher / your CFW's home screen.

### Custom border

Place the border image into `sd:/3ds/data/snes9x_3ds/`, under the name Border.png, your border image needs to be exactly 400x240.

### Custom top screen

Place the image into `sd:/3ds/data/snes9x_3ds/`, under the name top.png, your border image needs to be exactly 400x240.


_Feedback and bug reports are welcome. Help with development is also welcome!_

-------------------------------------------------------------------------------------------------------

## Screenshots

![Snes9x for 3DS](https://github.com/ramzinouri/snes9x_3ds/blob/master/assets/Screenshots_games.png?raw=1)

-------------------------------------------------------------------------------------------------------

## What's supported

1. Graphic modes 0 - 7.
2. Save states of up to 4 slots
3. Cheats - place your .CHT/.CHX (text format) with the same filename in the same folder as your ROM. For example, if your ROM name is MyGame.smc, then your cheat file should be named MyGame.CHT or MyGame.CHX
4. Currently uses CSND for audio. So your entry point to home-brew must be able to use CSND. If you can play BlargSNES with sound, you should be able to play Snes9X_3DS with sound.
5. Frame skipping.
6. Stretch to full screen / 4:3 ratio
7. PAL (50fps) / NTSC (60 fps) frame rates.
8. Navigation of ROMs in different folders.
9. SDD1 chip (Street Fighter 2 Alpha, Star Ocean)
10. SFX1/2 chip (Yoshi's Island, but slow on old 3DS)
11. CX4 chip (Megaman X-2, Megaman X-3)
12. DSP chips (Super Mario Kart, Ace o Nerae)
13. SA-1 chip (Super Mario RPG, Kirby Superstar)
14. Use of full clock speed in the New 3DS.
15. Sound emulation (at 32KHz, with echo and gaussian interpolation)
16. Automatic saving and reloading of savestates.

## What's missing / needs to be improved

1. Minor sound emulation errors.
2. Mosaics.
3. In-frame palette changes - This is because this emulator uses the 3DS GPU for all graphic rendering. Without in-frame palette changes implemented, a small number of games experience colour issues.

-------------------------------------------------------------------------------------------------------

## Frequently Asked Questions

### Why do some games have strange color issues (for eg., Wild Guns, Kirby Super Star, Judge Dredd, Batman Forever, Secret of Mana)?

Try going to the Settings and change the In-Frame Palette Changes to either one of the 3 options: Enabled, Disabled Style 1, Disabled Style 2. Color emulation is never perfect because we are using the 3DS GPU for rendering, which doesn't allow us to do what the SNES requires.

### Why some games keep writing to the SD Card every second or so (for eg., Treasure Hunter G, Some Super Mario Hacks, Final Fantasy Mystic Quest)?

Try going to the Settings and change the SRAM Auto-Save Delay to 10 seconds, 60 seconds, or disable it entirely! Don't worry, the SRAM will be saved again when you tap on the bottom screen.

### Can I use my save states generated from Windows versions of Snes9x?

You can try using save states from Snes9x v1.43, but sometimes this emulator doesn't recognize them.

### How can I make Snes9x utilize the full speed of the New 3DS?

The .3DSX version currently uses the full clock speed of the New 3DS, enabling all your SuperFX, SA-1 games to run faster.

-------------------------------------------------------------------------------------------------------

## .CHX Cheat File format

The .CHX is a cheat file format that you can create with any text editor. Each line in the file corresponds to one cheat, and is of the following format:

     [Y/N],[CheatCode],[Name]

1. [Y/N] represents whether the cheat is enabled. Whenever you enable/disable it in the emulator, the .CHX cheat file will be modified to save your changes.
2. [CheatCode] must be a Game Genie or a Pro Action Replay code. A Game Genie code looks like this: **F38B-6DA4**. A Pro-Action Replay code looks like this: **7E00DC04**.
3. [Name] is a short name that represents this cheat. Since this will appear in the emulator, keep it short (< 30 characters).

The .CHX must have the same name as your ROM. This is an example of a .CHX file:

Filename: Gradius III (U) [I].CHX
```
Y,F38B-6DA4,Start with 31 lives
Y,7E00DC04,Constant Megacrush (enemies die)
```

If you are still lost, refer to a sample Gradius cheat file here:
https://raw.githubusercontent.com/bubble2k16/snes9x_3ds/master/samples/Gradius%20III%20(U)%20%5B!%7D.chx.txt

NOTE: You can only have either .CHT or .CHX in your ROM folder for the same ROM. If you have both, only the .CHX file will be effective.

-------------------------------------------------------------------------------------------------------

## How to Build

You will need:
- devkitARM r46
- [libctru](https://github.com/smealum/ctrulib) (latest commit)
- [citro3d](https://github.com/fincs/citro3d) (latest commit)

Then build by using "make".

To build a debug version use "make DEBUG=1".

*Please note that building with libctru v1.2.0 causes the emulator to run at Old 3DS clock speeds on a New 3DS. It seems like a known problem with libctru v1.2.0.

-------------------------------------------------------------------------------------------------------

## Credits

1. Snes9x team for the fantastic SNES emulator
2. StapleButter / DiscostewSM for their work on BlargSNES. Many ideas for optimisations came from them.
3. Author of SF2D library. Some codes for hardware acceleration evolved from here.
4. Authors of the Citra 3DS Emulator team. Without them, this project would have been extremely difficult.
5. Maki-chan for helping with the libctru 1.2.0 conversion.
6. Fellow forummers on GBATemp for the bug reports and suggestions for improvements.
