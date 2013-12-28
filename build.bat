@echo off

for /f %%i in ('tools\win-bash\uname') do set OS=%%i
tools\Win-Bash\bash.exe build.sh -o %OS% %*