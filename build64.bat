@echo off 
set config=Release
set folder=_build64

if "%1" == "clean" goto CLEAN

@mkdir %folder% >nul
pushd %folder% >nul

@echo on

cmake -A x64 ..
vs_export -s DetoursCMake.sln -c "Release|x64"
mklink ..\compile_commands.json compile_commands.json

cmake --build . --config %config%

@echo off
popd >nul
goto END

:CLEAN
rd %folder% /s/q
goto END

:END
pause