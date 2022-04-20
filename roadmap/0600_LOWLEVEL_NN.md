# Lowlevel nn features

This document loosely explains all even more advanced nn features, specifically ones relating to the lowest levels of code (hell).

It is unlikely that I'll ever get here at the current rate of rewriting and rethinking the project, but at least the ideas are documented somewhere.

## Asm blocks
Blocks of good old assembly, and how it interacts with the rest of the program.

## Bitfields
Packed data in structs for optimal space efficiency.

## Alignment
Putting things at weird memory offsets for optimal work efficiency.

## Location
Demanding data exists at specific locations for MMIO and other such shenanigans.

## ABI
This is more of a concern for later, actually, but make a way to specify ABI on data. 