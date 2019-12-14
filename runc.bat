@echo off >nul
REM helper to build/run c program in one hit.
gcc -g -c arbitrage.c -o arbitrage.o && gcc -o arbitrage.exe arbitrage.o && arbitrage