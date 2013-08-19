# Kobo Deluxe Ports


This is currently a working repository for MacOS, Android and iOS porting work. The primary motivation for this work is simply that I'm a huge fan of this 2D retro space shooter game and thought it would be a good opportunity to learn [SDL](http://www.libsdl.org), while getting it to run on one or more of my mobile tablets.

### Repo structure
Each port are organized individually on branches as follows:

* **master** - This master branch has the raw Kobo Deluxe 0.5.1 source. It builds a command line app.
* **macos-port** - MacOS X 10.7+ (what's my deployment target??) with SDL v1.2. Dependent on SDLMain
* **macos-port-SDL2** - MacOS X  10.7+ with the RC for SDL v2.0
* **android-port** - Android 2.3.3+ port, optimized for Jelly Bean
* **ios-port** - iOS 5.0+ port with RC for SDL v2.0 (Note this latter version of SDL has good support for mobile oses)


### About

[Kobo Deluxe](http://olofson.net/kobodl/) is an enhanced [SDL](http://www.libsdl.org) port of Akira Higuchi's scrolling 2D space shooter game [XKobo](https://github.com/hatemogi/xkobo), for UN*X X Window systems. Written by [David Olofson](http://olofson.net), [Kobo Deluxe](http://olofson.net/kobodl/) adds sound, smoother animation, high resolution support, OpenGL acceleration, a menu driven user interface, joystick support. Kobo Deluxe includes many updates essential to tackling the enemy ships that shoot at you, chase you, circle around you shooting, or even launch other ships at you, while you try to destroy the labyrinth shaped space bases. 

[XKobo](https://github.com/hatemogi/xkobo) itself, is a simple open-source X11 video game developed by Akira Higuchi in 1997.



### License

 Kobo Deluxe - An enhanced SDL port of XKobo <br/>
 
 Copyright © 1995, 1996 Akira Higuchi <br/>
 Copyright © 2001-2003, 2005-2007 David Olofson <br/>
 Copyright © 2005 Erik Auerswald <br/>
 
```
  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the
  Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.
  
  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.
  
  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  675 Mass Ave, Cambridge, MA 02139, USA.
```
