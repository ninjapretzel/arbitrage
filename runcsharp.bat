@echo off >nul
REM Helper to clean and run C# version
rm *.exe & rm *.dll & mcs arbitrage.cs && mono arbitrage.exe