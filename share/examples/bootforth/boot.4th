\ Example of the file which is automatically loaded by /boot/loader
\ on startup.
\ $Id$

\ Load the screen manipulation words

cr .( Loading Forth extensions:)

cr .( - screen.4th...)
s" /boot/screen.4th" fopen dup fload fclose

\ Load frame support
cr .( - frames.4th...)
s" /boot/frames.4th" fopen dup fload fclose

\ Load our little menu
cr .( - menu.4th...)
s" /boot/menu.4th" fopen dup fload fclose

\ Show it
cr
main_menu
