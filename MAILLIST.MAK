# This file creates the MAILLIST.EXE program.
# Copyright (c) 1992, 1993, 1994 Fredric L. Rice
#
# The MODEL variable is used to describe
# the memory model to use.
#
# Compiler SWITCHES:
#    -G 	Generate for speed rather than size
#    -O 	Optimizes jumps
#    -d 	Merge duplicate strings
#    -Z 	This would optimize registers
#    -K		Default char is considered unsigned
#    -g1        Stop after first warning
#    -j1	Stop after first error
#    -w         Enable all warnings
#    -Ixxx      Include files directory
#    -Lxxx      Libraries directory
#    -N         Check stack overflow
#    -c         Compile only
#    -mx        Set Memory Model
#    -P         Force C++ Compile

MODEL = l

SWITCHES1 = -Ic:\tcc\include -G -O -d -Z -K -g1 -j1 -w -N -P

# Describe the files that are used to
# create the executable module.

MAILLIST.exe : MAILLIST.obj

# Here is the command to execute if
# uucp-ra.exe needs to be linked.

     tlink /x /i @MAILLIST.res

# The following are the commands that create the object module

MAILLIST.obj : MAILLIST.cpp MAILLIST.hpp
     tcc -c $(SWITCHES1) -m$(MODEL) MAILLIST.cpp

