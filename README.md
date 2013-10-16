Linux CLI For Rigol DS1000E(D) Oscilloscopes
============================================

A command-line interface tool for Rigol digital oscilloscopes.
Works with DS1000E(D) series, may also work with other models.

Based on work by Mark Whitis and [Jiri Pittner](http://www.pittnerovi.com/jiri/hobby/electronics/rigol/).

Kernel module
=============

USB Communication with the Rigol scope uses the usbtmc kernel module.
However, the Rigol firmware uses the USBTMC protocol in a non-standard way
(it sends large amounts of data in a singe transaction instead of 1024 chunks),
which required a
[modification in this kernel module](https://github.com/torvalds/linux/commit/50c9ba311402f611b54b1da5c6d49873e907daee).
This modification is part of linux 3.11.

For earlier versions, a patched version of the usbtmc kernel module
must be installed in order to communicate with Rigol equipment,
otherwise the scope's firmware will crash.
I currently use
[this version](https://github.com/tommie/linux/blob/704920711e31f973f5926ff2137ed5a80693a7b3/drivers/usb/class/usbtmc.c)
with a 3.5.0 kernel.

Commands
========

Commands are documented in the [DS1000E(D) Series Programming Guide](http://www.rigol.com/download/Oversea/DS/Programming_guide/DS1000E%28D%29_ProgrammingGuide_EN.pdf).

Examples:

    *RST
    :AUTO
    :TRIG:MODE EDGE
    :ACQ:MEMD LONG
    :CHAN1:MEMD?
    :TIM:SCAL 0.0001
    :ACQ:SAMP?
    :CHAN1:SCAL 1.
    :RUN
    :STOP
    :WAV:POINTS:MODE RAW
    :WAV:DATA? CHAN1

License
=======

* Copyright (C) 2010 Mark Whitis
* Copyright (C) 2012 Jiri Pittner
* Copyright (C) 2013 Ralf Sternberg

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see [http://www.gnu.org/licenses/].

