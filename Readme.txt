This code is based on the IsDcc2.1.
You can still see the information here and there.

I have rewritten parts of the code but I haven't taken the time to clean or fix everything.
There might be some code left that was dealing with weird data before I took the labels into account.

However it should provide a good start to someone wanting to make a full version.

It should output the scripts fine but there might be some unknown opcodes left.

I only tested with 3 scripts (2 from InstallShield12 (Trial version) and one from InstallShield 2010 (trial version) )
The opcode 0x3B is still unknown and the output of some other opcode might be wrong.

I have not taken the time to fix the global variable part but it should not really matter.

Ok so how to use it?

First you need to unscramble the .inx script
For that launch the application with: isdcc31 -u setup.inx
The will generate a setup.inx.dec

Then decode the script using : isdcc31 setup.inx.dec

If you, by any chance, need to patch the dec file (see decodetable.h and other source code for the opcodes), you will need to scramble it again before using it.

To scramble the file: isdcc31 -s setup.inx.dec
This will create a setup.inx.dec.inx :) 
Up to you to rename it and keep a backup of the original inx file.

Enjoy.

Basic Opcode structure:

opcode (2 bytes) nbofParams (2bytes) nb * [ typeofParam (1byte) + paramdata (variable) ] 


