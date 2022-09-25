This is a copy of [wiki page](https://github.com/ramok/xxkb/wiki). Check it out for recent changes.

***

## Introduction
The xxkb program is a keyboard layout switcher and indicator.
Original [README](https://github.com/ramok/xxkb/blob/master/README) for full information.

This is converted from CVS repository from [sourceforge.net](https://sourceforge.net/p/xxkb/).
While project from sourceforge looks like abandoned, this repository is collection useful patches from everywhere.

## Changes
* Some Debian package patches merged
* Add option `XXkb.keymask.cycle`. Usefull if you have more then two layouts. This added possibility to use hotkey for changing layouts in cycle. Otherwise you forced use mouse for this.

  For example, if you have `CapsLock` as layout switch adding this option make `Ctrl-CapsLock` cyclic chaining layouts.
`~/.xxkbrc`
```
   XXkb.keymask.cycle: ctrl
```
* Add [patch](https://github.com/ramok/xxkb/commit/a75f6b136278e778254ea481a9f405886082a487) 'Add layout state output to another process' by [Vladimir Rudnyh](https://sourceforge.net/u/dreadatour/profile). [Original patch](https://sourceforge.net/p/xxkb/patches/3/).

  Usefull to show up layout status in **dzen2**, **xmodbar** and so on in tiling windows managers like **xmonad**, **awesome**, ...
* Add [Estonian language flags](https://sourceforge.net/p/xxkb/feature-requests/6/) by [Dknight](https://sourceforge.net/u/dknight1/profile/)
* Add Poland language flags from Gentoo port
* Add [Czech language flags](https://github.com/ramok/xxkb/pull/3) by [ANtlord](https://github.com/ANtlord)

