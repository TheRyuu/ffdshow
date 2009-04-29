REM command line used to build libboost_thread-vc90-mt-s-1_39_52655.lib

bjam --toolset=msvc-9.0 address-model=64 --with-thread variant=release link=static threading=multi runtime-link=static stage
