REM command line used to build libboost_thread-vc90-mt-s-1_44_64745.lib
REM toolset = msvc-9.0 | msvc-10.0
REM variant = release | debug

bjam --toolset=msvc-9.0 address-model=64 --with-thread variant=release link=static threading=multi runtime-link=static stage
