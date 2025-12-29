@echo off
echo === Prepare environment ===

rem 1. Copy server binary
echo Copy server binary...
copy ..\..\build\Debug\server.exe .\server.exe

rem 2. Install deps
echo Install deps...
call npm install

rem 3. Install playwright browsers
echo Install playwright browsers...
call npx playwright install

echo === OK! Run the tests... ===
call npm test

echo Clear environment...
del server.exe chat.db