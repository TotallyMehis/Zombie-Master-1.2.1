***************************************
Zombie Master Beta 1.2.1 
Source code release README
Last updated: 13-06-09 (dd-mm-yy)
www.zombiemaster.org
***************************************

This is the source code of Zombie Master, based on the Source SDK 
("HL2 Episode 1" version). For questions, discussion, etc., please visit 
the forums at www.zombiemaster.org

Some basic information is contained in this document.

***************************************
 1. Licensing
***************************************
The modifications made to the SDK that form Zombie Master are released under
the MIT license. See the included LICENSE.txt for details on what is and is
not allowed. Generally, you are free to modify the code and release the result.

The ZM development team would appreciate it if credit is given when you use
significant amounts of our code in your project.

Note that all terms of the license apply exclusively to the Zombie Master code.
In order to (for example) redistribute the Zombie Master maps, models,
textures, and other art assets, you will need to get separate permission from
the respective authors.

Also note that the license applies only to the *modifications* made by us to
the original SDK code, and any *new* code written by us. All original SDK
code remains under Valve's copyright and licensing.

We have attempted to add clear license notices to all source code files that
are either newly added to the SDK codebase for ZM, or contain large changes
made by the ZM team to the original code. The license also applies to all
modified or new code contained in files that do not explicitly show a notice, 
but do contain code produced by the ZM team. Such files only lack a notice 
because it is not feasible to add it to all files with changes.

The MIT license can also be found at the following URL:
http://opensource.org/licenses/mit-license.php


***************************************
 2. Usage
***************************************
Notes on working with the code:

- Visual Studio .NET 2003 is required, later versions have not been tested, 
  though with some modifications VS 2005 might work. Updates for VS 2005
  compatibility may be available in the zombiemaster.org forums in the future.

- You will have to modify the various directory paths used in the compilation
  process. See the "Custom Build Step" section in the properties of the two
  projects contained in the solution.
  
- See the Valve Developer Community for more information on compiling the
  Source SDK code.
  
- Only the "Release" build configuration is likely to work, because ZM was
  programmed by cowboys (see below).

- Prepare to see lots of code written with the aim of achieving a functional
  state as fast as possible, in order to squeeze as many fixes and features
  in a limited amount of available programming time. This is another way of
  saying it is not necessarily pretty, clean, or a realistic depiction of the
  programming abilities of the people who wrote it.

- In order to play what you have compiled, you will of course require a
  Zombie Master 1.2.1 installation. These files are not included in this
  release (or the licensing scheme), but can be found at the ZM site or in
  many other places.
  
- If you release your changes as a patch/update/new mod, you should refrain
  from naming it "Zombie Master". Instead, add a sub-title or change the
  name entirely. This is necessary to avoid confusion between different
  versions of the mod by different authors, including the original ZM version.
  No one benefits from a situation where there are fifty incompatible versions
  all called "Zombie Master 1.X.X". A subtitle such as 
  "Zombie Master: Beachball Extreme 1.0" helps a lot.
  
- For more and/or updated information, visit the forums at zombiemaster.org
  
***************************************
 3. About
***************************************
Zombie Master was programmed by Angry Lawyer, theGreenBunny, and qbb.

See the readme included in the Zombie Master 1.2.1 release for a full list of
team credits, including non-programming contributors.
