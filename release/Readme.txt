JoySens - Analog stick sensitivity plugin for custom firmwares
version 1.5

(C) 2007-2010 Raphael <raphael@fx-world.org>




1. How to install

Just unzip the archive to your memory stick root. You may delete the Readme.txt and FAQ.txt if you so want.

Hard reboot your PSP into recovery menu (hold R Trigger during startup) or start from VSH if you have the proper
recovery menu plugin installed and enable the joysens or joysens_lite plugin. Done.


If you get a message about overwriting a .txt file (you already have other plugins installed) tell not to
overwrite and manually edit the following files in your ms0:/SEPLUGINS/ folder by appending the line
'ms0:/SEPLUGINS/joysens.prx' and/or 'ms0:/SEPLUGINS/joysens_lite.prx' (without quotes):
	- game.txt for UMD games and homebrew loaded from GAMEyxx folder (or GAME folder in y.xx kernel mode)
	- game150.txt for homebrew loaded from the GAME150 folder (or GAME folder in 1.5 kernel mode)
	- pops.txt for games played with POPS
	- vsh.txt for VSH/XMB

If you're running 3.71 custom firmware you're pretty much out of luck - the plugin will only work with 1.5 kernel addon
in game150 mode. Upgrade to a newer custom firmware >= 3.80, which includes a NID mapper, that allows
plugins to work no matter if function NIDs were changed in the official firmware.



2. Usage

This plugin changes the analog sticks sensitivity to more useablity in the following ways:
- It changes the axial mapping to a cubic (or other selected) form so that small movements don't get recognized too early.
  This means that you can move cursors more exactly and don't have it jumping around on the screen.
  Also, this greatly reduces and fixes the 'ghosting' of a faulty analog stick.
- It applies a sensitivity scale to linearly change the responsiveness (and maximal amplitude for values < 100%)
  of the stick. With this you can 'slow down' the analog stick.
- It filters movement of the analog stick so that abrupt changes are further smoothed (adjustable).
- It is possible to calibrate the analog stick so it centers properly
- It is possible to rescale the analog stick so it returns correct ranged values (when analog doesn't work in one or two directions any more)
- It allows to remap the DPad or round buttons to the analog, so you can play games that require analog input with a broken stick
- It allows to remap the analog to DPad or round buttons, so you can for example control the XMB with the analog
- It allows to swap analog and DPad or round buttons, so you can easily change control of your games from analog to DPad (or round buttons) while still having DPad (round buttons) input

This makes the analog stick more useable for high-precision needs like FPS or anything where you move something
with the analog stick that needs to be placed next-to-pixel-precise.

Joysens also comes in two versions, a normal and a lite version. The only difference is, that the lite version doesn't support
config file saving, ingame info output and ingame settings changing with button presses, and therefore is much smaller in size
and ram/cpu usage. If you have problems running the normal version, it is recommended to try the lite version. 
However, for changing any settings or creating new configs for specific games, you need to edit the joysens.ini file yourself.


OPTIONS

ENABLE -
	To turn the plugin on/off at any given time press NOTE + SELECT.

SENSITIVITY - 
	To change sensitivity press NOTE + LEFT TRIGGER or NOTE + RIGHT TRIGGER.
	Default sensitivity is 100%. The minimum is 10% (which effectively makes the analog stick unuseable)
	and maximum is 400% (which is way sensitive).

SMOOTH -
	To change smoothness of movement press NOTE + LEFT or NOTE + RIGHT.
	Default smooth is 100%. The minimum is 10% (which effectively makes the analog stick unuseable)
	and maximum is 400% (which is way sensitive - the smallest change will give maximum amplitude already).

ADJUST -
	To change the adjust function exponent press NOTE + UP or NOTE + DOWN.
	Default is 3. Minimum is 0 (which really turns the analog stick off) and maximum is 32 (which only gives
	a response when the stick is moved to its extremes, making it quasi-digital). The higher this value the better
	the deadzone gets 'hidden'.
	A value of 1 will result in normal behaviour and anything < 1 will probably only make the stick behave worse.
	If you want to get an imagination of what you are setting with this, plot the function x * (x/127)^(a-1) in
	the range 0 to 127 for your chosen adjustment value a. If you don't have a clue about this, just forget it :P

CENTER -
	To change the calibration either change it manually in the ini file (center) or try the following:
	- Move the analog stick to the top-left and hold it a few seconds, then release slowly until it's centered and don't
	  touch it anymore. Press NOTE+SQUARE,
	- Move the analog stick to the bottom-right and hold a few seconds, release slowly until centered. Press NOTE+SQUARE.
	- Repeat these steps until the center point has been found correctly (optimally, this is the mean value of the axes
	  minimal and maximal value when the stick isn't touched).
	Default value is <0,0>.

RESCALE -
	To change the analog scaling either edit the min and max values in the ini file or try the following:
	- Press NOTE+SCREEN. The values in the second to last line of the info should change to something different than
	  (-128,-128)->(127,127).
	- Move the analog stick to the extremes a few times.
	- Press NOTE+SCREEN. Optimally the second to last line of the info should now still display something different,
	  but more close to (-128,-128)->(127,127) but only if your analog stick is really faulty.

FORCEANALOG -
	You can select to force analog stick input to be enabled. This is useful for the XMB to be able to control it with analog.
	It does not harm any games that already poll analog input, hence why there is no enable/disable button combination (you
	can assign a button to that function though in the ini file).

RESET -
	To reset all values to default press NOTE+CROSS.

INFO -
	To pin/unpin the info output press NOTE + TRIANGLE. This will make the info output stay on top of the screen
	even when no button is pressed.

	NOTE: The info output does NOT work in all games (though it does in most and in XMB), so even when you don't see anything,
		  the plugin might still be running!
		  To check that, set the adjust parameter to 0 and see if the analog stick is turned off ingame. If so, the plugin
		  works and only the info output isn't.
		  If you don't get any info output even in XMB, then the plugin doesn't work. Check that you installed and enabled
		  the plugin correctly in your CFW's recovery menu.

	If not pinned to show the current settings only press and hold NOTE.
	You should see an output on the top-left of the screen like this:

	Adjusted analog axes: <16,12> -> <0,0>
	JoySens: on (info pinned)
	Sensitivity: 100%
	Smooth: 100%
	Adjust: 3.0
	Center: <0,0> (127,127) -> (-128, -128)
	Remap: Analog->DPad

	The first line is important to check if the analog stick properly maps the axes. Optimally the second coordinate pair
	should be constantly <0,0> if you don't touch the stick (while the first pair might jump around) and still give the full
	range from -128 to 127 in both axes (unless sensitivity < 100%) when you move the stick to the extremes.
	This line doesn't give the proper values in XMB other than photo viewer, browser and system information as XMB doesn't poll
	the analog stick unless you enable FORCEANALOG (set to 1).
	The second to last line gives the current center offset plus the current set minimum and maximum amplitude to rescale input (gets
	updated with NOTE+SCREEN). They only should matter to you when your analog doesn't (or very slowly) respond in one or two directions.
	The last line shows the current remapping method. See below for more information.

REMAP -
	The first option is to map DPad movement to analog stick (will make DPad unuseable for other uses), in case your analog is
	completely useless but a game requires analog input. Press NOTE + START to toggle the remapping.
	The second setting will remap the analog stick to DPad (analog will still be interpreted) so you can control for example
	the XMB with the analog stick.
	The third setting will swap the analog stick and DPad input, so you control the XMB with analog ONLY for example, while
	photo viewer and browser will get controlled by the DPad. This is mainly useful for games where you prefer DPad over
	analog control, but the game doesn't allow such a remapping and you still need analog input as well.
	The last setting will turn remapping off (default).
	
	Mode #	|  Mapping				| Info
	---------------------------------------------------------------------------------------------
	0		|  None					| PSP behaves as normal
	1		|  DPad->analog			| DPad presses get recognized as analog pushes too
	2		|  Analog->DPad			| Analog pushes get recognized as DPad presses too
	3		|  Analog<->DPad		| Analog and DPad are swapped
	4		|  Buttons->analog		| Round button presses get recognized as analog pushes too
	5		|  Analog->buttons		| Analog pushes get recognized as round button presses too
	6		|  Analog<->buttons		| Analog and round buttons are swapped

REMAPMODES -
	This is an option only set within the joysens.ini file and will select a number of modes you want to switch between in the order given.
	For example, setting REMAPMODES = 0,2,6 will switch between no remapping, Analog->DPad and Analog<->buttons modes.
	However, you can also set REMAPMODES = 1,0,4 to switch from DPad->analog to none and then Buttons->analog remap mode.
	It makes sense to set the REMAPMODES in VSH to 0,3 or 0,2 in order to control the VSH with analog but be able to easily
	switch to no remapping to properly control the browser or image viewer with analog stick again.

SAVE -
	To save the current settings press NOTE + CIRCLE.
	This will create a new settings section if you're in a game that isn't yet listed. From then you can configure
	JoySens specifically for this game too.


Also, you can change the button mapping in the joysens.ini file by properly setting the buttons. Just make sure there
is no double mapping!
Possible are SELECT,START,RTRIGGER,LTRIGGER,TRIANGLE,CROSS,CIRCLE,SQUARE,LEFT,RIGHT,UP,DOWN,SCREEN,NOTE,VOLUP,VOLDOWN.
Any different value will turn off that function for online-adjustment.
You can also change the primary button mapping from NOTE to something else in case you already have NOTE mapped for another
plugin, or set the primary button to nothing ("primarybtn =") to completely disable settings changes with button presses.



3. Todo

- Better support for ingame info output (Find a better function hook than sceDisplaySetFrameBuf as some games seem to
  bypass that function - namely GTA:VCS and POPS games).
- allow button combinations in joysens.ini
- more flexible remapping, with hotkey button combinations maybe (L + Dpad -> Analog)

This plugin was tested under CFW 3.10-OE-A', 3.40-OE-A, 3.51-M33-7, 3.71-M33 and 4.01-M33, as well as in the games GTA:VCS,
CoD:RTV, RidgeRacer and Tekken5:DR.



4. Updates

from v1.42b
- code cleanup
- added possibility to choose only from a selected list of remapmodes (ie only switch between your favorite modes) per game/vsh
- fixed that adjust=0 will turn off the analog
- added LITE version, that does not support config file saving, no ingame info output as well as no ingame config settings with buttons
- added "threshold" config parameter, to adjust button remapping from analog
- added "thresholdupbtn" and "thresholddownbtn" config parameters, to set buttons for setting the threshold ingame
- added "idlestop" and "idleback" config parameters, to set when the analog movement stops the idle timer or returns back from idle
  (brightness dimming/display off feature of PSP firmware).
- optimized config file reading a bit
- updated FAQ

from v1.42
- fixed saving settings bug killing the .ini file

from v1.41
- fixed compatibility issues with Sony UMD driver (and possibly some other applications that require more kernel memory)
  The config file system now uses a mere 1Kb of RAM where it used 24+Kb in 1.4/1.41
- Reduced module size a bit (to further help memory problems)
- added a workaround info output for POPS (flickers a lot, but at least you see something)
- fixed the adjust calculation to avoid crashes for high values (shouldn't happen anymore even with adjust 32.0)
- fixed a little Button remapping bug


from v1.4
- fixed crash when saving settings from game
- fixed JoySens not starting in homebrew
- made JoySens not boot in recovery


from v1.3
- Added support for configs for different modes (VSH/POPS) and game/eboot paths
- Added analog<->Buttons mapping modes
- Fixed primary button mapping
- Fixed HOLD button preventing analog input
- Fixed to keep PSP from suspending/shutting down LCD if analog was pressed
- Fixed font output shadow making it better readable on light background
- Improved auto-center function a bit
- checked compatibility with CFW up to 4.01-M33-2
- Made installation easier


from v1.2
- Added a rescaling functionality to fix analog sticks that return too small values in either direction (hence
  the analog stick wouldn't react in that direction)
- Added DPad/analog swapping
- Made plugin executable smaller by reverting to kernels libc and implementing a fake pow() function rather than linking libm
- Ensured compatibility with homebrew (actually needs the plugin enabled in game150.txt/game3xx.txt depending on the set kernel mode)
- Added FAQ.txt for some frequently asked questions. READ IT BEFORE ASKING QUESTIONS!
- Fixed a minor bug that could cause the plugin and the PSP to crash when using the auto-center function


from v1.11:
- Fixed a major bug with the calibration function that scaled the range wrongly (should now really be useable)
- Added a function to map the DPad buttons to analog in case you can't fix your analog stick but a game requires analog stick
- Added a function to force analog input (for XMB for example)
- Added a function to map the analog to DPad (to control XMB with analog)
- Added button mappings for SCREEN (brightness), VOLUP and VOLDOWN


from v1.01:
- Added better calibration method (keeps the axis scale in range)
- Fixed automatic calibration function (gives pretty reliable results now) and moved vertical and horizontal calibration into
  one function
- Added a movement smooth factor (how much changes in the axes get scaled, the lower the smoother any movement gets recognized)
- Added an adjust parameter which controls the exponent of the adjustment function (default 3.0 meaning a cubic form)
- Added button mapping configuration
- Fixed a small error in the axis mapping, where values were mapped in range -127 to 128 where it should be -128 to 127
- Added setting save functionality
- Improved the readme with better instructions


from v1.0:
- Added compatibility for 3.40 OE and higher firmwares



5. Contact

http://wordpress.fx-world.org
raphael@fx-world.org



6. Thanks&Greets

Thanks to all who made the PSPSDK possible.
Special thanks to TyRaNiD for PSPLINK. The main function hook functionality is based on remotejoy's hook function.
Big thanks to Dark_Alex for creating the NID resolver in firmwares >= 3.80. Saves us plugin creators heaps of work
(and the users a mess of different version plugins).

Greets to the people from ps2dev.org and psp-programming.com



7. Disclaimer

This software was not produced by Sony Corp nor any of its affiliates or representatives.
It is not endorsed by Sony, therefore they hold no responsibility for the use of this modification with their product.
PlayStation is a registered trademark of Sony Computer Entertainment, to which I have no affiliation.

Use at your own risk. I may not be held responsible for any harm that this software causes to your PSP.


