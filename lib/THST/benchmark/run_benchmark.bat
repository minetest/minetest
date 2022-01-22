@echo off
REM ############################################################################
REM Script running spatial_index_benchmark programs
REM https://github.com/mloskot/spatial_index_benchmark
REM ############################################################################
REM Copyright (C) 2013 Mateusz Loskot <mateusz@loskot.net>
REM 
REM Distributed under the Boost Software License, Version 1.0.
REM (See accompanying file LICENSE_1_0.txt or copy at
REM http://www.boost.org/LICENSE_1_0.txt)
REM ############################################################################

IF NOT EXIST "%1" GOTO :NOBUILDDIR

for /r %%f in (%1\*.exe) do (
    @echo Running %%~nf
    %%~ff > %%~nf.dat
)
GOTO :EOF

:NOBUILDDIR
@echo Cannot find '%1' build directory