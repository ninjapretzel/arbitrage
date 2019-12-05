#!/bin/bash
#helper to clean and run java version on non-windows. ymmv.
rm *.exe & rm *.dll & mcs arbitrage.cs && mono arbitrage.exe