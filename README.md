# MiniMake
Simple MAKE for use with TCC.

This is the source distribution for the "make" program that
was included in MINIX versions 1.7 through 2.0.4, the latter
of which it was taken from.

The reason for reviving this code was to get a reasonably
"able" version of Make for use with the small-and-fast TCC
compiler.

Some effort was put in to remove MINIX-specific things, and
support for very old systems we no longer want to support. The
code has been converted to be fully ANSI-compliant, and, where
this was possible and useful, 'const' was added to pointers.

The rules in 'rules.c' are configured specifically for Windows
and UNIX (which of course means Linux and alikes, plus, these
days, Apple macOS.)  It further checks which compiler is being
used to compile it, and sets things up accordingly.

Supported:

 Windows:
   Microsoft Visual Studio (_MSC_VER defined)
    -> set CC to "cl", and so on, objects to ".obj"
   Tiny C Compiler (__TCC__ defined)
    -> set CC to "tcc", optimize to "-O2", objects to ".o"
   MinGW and others:
    -> set CC to "cc", optimize to "-O", objects to ".o"

 UNIX:
   Tiny C Compiler (__TCC__ defined)
    -> set CC to "tcc", optimize to "-O2", objects to ".o"
   Others:
    -> set CC to "cc", optimize to "-O", objects to ".o"

As it is now, the program seems to be working fine. When compiled
with TCC, the resulting binary is about 30KiB !

Things not currently implemented, but which can be added:

- if/else/endif and friends
- VPATH
- MAKEFILE_LIST macro
- macro functions, such as dir(), abspath(), firstword(), lastword()
  and a few like that

Most of these should be fairly trivial to add.

For suggestions/comments etc, contact me, NOT any of the original
authors - they didn't do it, and, thus, are innocent! :)


Fred N. van Kempen
waltje@varcem.com

Last Updated: 2023/06/01
