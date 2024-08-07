@echo off

setlocal enableextensions enabledelayedexpansion

REM Prompt the user to input the new file name
set /p new_project_name=Enter new Project:

REM Set source and destination folder paths
set source_folder=template
set destination_folder=samples/%new_project_name%

REM Copy folder and all its contents
xcopy "%source_folder%" "%destination_folder%" /E /I

REM Prompt the user to input the new file name
rem set /p new_file_name=Enter new file name (without extension):

REM Rename specific files
set "inputFile=%destination_folder%\template.sln"
set "outputFile=%destination_folder%\%new_project_name%.sln"

for /f "delims=" %%a in ('type "%inputFile%"') do (
  set "line=%%a"
  set "line=!line:template=%new_project_name%!"
  echo !line!>>"%outputFile%"
)
del "%inputFile%"


set "inputFile=%destination_folder%\template.vcxproj"
set "outputFile=%destination_folder%\%new_project_name%.vcxproj"

for /f "delims=" %%a in ('type "%inputFile%"') do (
  set "line=%%a"
  echo !line!>>"%outputFile%"
)
del "%inputFile%"


echo New project generated successfully.