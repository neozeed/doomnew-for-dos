![output](https://github.com/neozeed/doomnew-for-dos/assets/9031439/c3fc620e-4292-42cc-96a2-b157cbfc59ef)

DoomNew is a enhanced DOS source port created by Frank Sapone, otherwise known as Maraakate. It is derived from Linux Doom. It aims to enhance the stock executable with system drivers and bug fixes derived from Heretic and new features, including the use of the DMX sound engine, internal and external DeHackEd support and the raising of static limits, such as visplanes.

History

Frank Sapone was previously active in the Daikatana community and impressed John Romero with the community patch for it that improved the stability of the game. Wanting to make an improvement to Doom, he acquired the DMX sound engine from Romero himself. The first activity was on December 19, 2012, and DMX was linked on June 1, 2013. The port is known as Doom 1.9a. Sapone kept a detailed record of his progress in a file called CHANGES.TXT.

September 2013 was easily the most productive month for the port. On September 9, music was added in, with graphics and sound following on the 10th. From the 12th to the 20th, the port was brought up to speed, gaining support for all versions of Doom, along with TNT: Evilution, The Plutonia Experiment, Perdition's Gate, Hell to Pay, Chex Quest and HacX. A lot of effort was spent taking code from Sapone's other port: an enhanced version of Heretic. On September 24, preliminary DeHackEd support, adapted from Chocolate Doom, was added in. On October 10, experimental support for internal DeHackEd patches was added.

2013 and 2014 were thus spent on fixing bugs, partially using code from his enhanced Heretic fork. After November 11, 2014, development stopped until August 31, 2015 when Sapone released a massive code clean up. Experimental support for the SimulEyes Virtual reality glasses was provided utilizing a secondary executable called DOOMVR.EXE. The last update and release was on October 21, 2015.

Features
* Utilizes the DMX sound engine
* Support for Doom, The Ultimate Doom, Doom 2, TNT: Evilution, The Plutonia Experiment, Perdition's Gate, Hell to Pay, Chex Quest and HacX through their own parameters
  -doom, -doomu, -doom2, -tnt, -plutonia, -perdgate, -hell2pay/-helltopay, -hacx
* A additional configuration file, called EXTEND.CFG. With this you can set parameters:
  -novert to disable vertical movement
  -noprecache to disable graphics precaching
  -nowipe to disable the screen melt effect
* Doom.wad will search for Episode 4 and auto-load The Ultimate Doom. This can be overridden using the -doomu parameter
* Specify the main WAD file utilizing the -mainwad parameter
* Ability to load older save files using the -oldsave parameter
* Ability to convert older save files utilizing the -convertsave <slot #> parameter
* Various new cheats from Heretic 1.3a:
  MASSACRE
  HHSCOTT: This instantly finishes the level
* Speed enhancements from Heretic, such as linear.asm
* Raised limits:
  SAVEGAMESIZE to 1024 KB
  SAVESTRINGSIZE from 24 to 256
  Visplane limits are removed, utilizing code from wHeretic
* Support for internal and external DeHackEd patches
* Ability to toggel loading internal DeHackEd files through the -deh_internal parameter
* Virtual reality support through DOOMVR.EXE
