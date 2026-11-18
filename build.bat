@echo off
REM build.bat - builds without make. Usage: build.bat   (then: library.exe  /  run_tests.exe)
set SRC=src\datagen.cpp src\divide_conquer.cpp src\string_match.cpp src\greedy.cpp src\dp.cpp src\backtrack.cpp src\graph.cpp src\library_engine.cpp src\io.cpp src\bench.cpp src\timer.cpp src\scenarios.cpp
g++ -std=c++17 -O2 -Wall -Wextra -Iinclude %SRC% src\main.cpp -o library.exe || exit /b 1
g++ -std=c++17 -O2 -Wall -Wextra -Iinclude %SRC% tests\test_main.cpp -o run_tests.exe || exit /b 1
echo Build OK: library.exe and run_tests.exe
