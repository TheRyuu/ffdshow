@ECHO OFF
rem command line used to build libboost_thread-vcTOOLSET-mt-VARIANT-1_46_1.lib
rem You need to build bjam first running bootstrap.bat and the appropriate MSVC version
rem toolset = msvc-9.0 | msvc-10.0
rem variant = release | debug

rem ECHO bjam --toolset=msvc-9.0 address-model=64 --with-thread variant=debug link=static threading=multi runtime-link=static stage
rem bjam --toolset=msvc-9.0 address-model=64 --with-thread variant=debug link=static threading=multi runtime-link=static stage

rem ECHO bjam --toolset=msvc-9.0 address-model=64 --with-thread variant=release link=static threading=multi runtime-link=static stage
rem bjam --toolset=msvc-9.0 address-model=64 --with-thread variant=release link=static threading=multi runtime-link=static stage

ECHO bjam --toolset=msvc-10.0 address-model=64 --with-thread variant=debug link=static threading=multi runtime-link=static stage
bjam --toolset=msvc-10.0 address-model=64 --with-thread variant=debug link=static threading=multi runtime-link=static stage

ECHO bjam --toolset=msvc-10.0 address-model=64 --with-thread variant=release link=static threading=multi runtime-link=static stage
bjam --toolset=msvc-10.0 address-model=64 --with-thread variant=release link=static threading=multi runtime-link=static stage
