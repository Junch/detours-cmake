@echo off 
set config=Release
set folder=_build64

if "%1" == "clean" goto CLEAN

@mkdir %folder% >nul
pushd %folder% >nul

@echo on

cmake -A x64 ..
vs_export -s DetoursCMake.sln -c "Release|x64"
del %~dp0compile_commands.json
mklink %~dp0compile_commands.json %~dp0%folder%\compile_commands.json

cmake --build . --config %config%

@echo off
popd >nul
goto END

:CLEAN
rd %folder% /s/q
goto END

:END
pause