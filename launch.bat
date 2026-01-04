REM Build
cmake -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE --no-warn-unused-cli -S . -B ./build
cmake --build ./build --config Release --target ALL_BUILD -j 12 --

REM Unit test
.\build\Release\test_server.exe

REM Integration tests
cd tests/integration
call run-tests
cd ../..

REM Clear test environment
del test.db

REM Start app
.\build\Release\server.exe