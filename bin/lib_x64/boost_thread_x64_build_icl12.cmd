@ECHO OFF
rem command line used to build libboost_thread-vcTOOLSET-mt-VARIANT-1_51.lib
rem You need to build bjam first by running bootstrap.bat
SET toolset=intel-12.1

FOR %%L IN (
  "debug" "release"
) DO (
  CALL "%ICPP_COMPOSER2011%..\ips-vars.cmd" ia32 vs2010
  CALL :SubBjam %%L 32 thread
  CALL :SubBjam %%L 32 date_time
  CALL "%ICPP_COMPOSER2011%..\ips-vars.cmd" ia32_intel64 vs2010
  CALL :SubBjam %%L 64 thread
  CALL :SubBjam %%L 64 date_time
)
PAUSE
EXIT /B


:SubBjam
ECHO.
ECHO bjam --toolset=%toolset% address-model=%2 --with-%3 variant=%1 link=static threading=multi runtime-link=static stage
bjam --toolset=%toolset% address-model=%2 --with-%3 variant=%1 link=static threading=multi runtime-link=static stage
ECHO.
EXIT /B
