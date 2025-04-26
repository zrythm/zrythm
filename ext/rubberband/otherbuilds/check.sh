#!/bin/bash
set -eu

if [ ! -d /Applications ]; then
    # Assumed to be Linux

    echo " *** Building static library using Linux-specific Makefile"
    make -f otherbuilds/Makefile.linux clean
    make -f otherbuilds/Makefile.linux
    
    echo " *** Linking against static library"
    g++ -std=c++11 main/main.cpp lib/librubberband.a -I. -Isrc -o test_static -lsndfile -lpthread
    
    echo " *** Running build from Linux-specific Makefile"
    ./test_static -V
    
    echo " *** Building with single-file source"
    g++ -O3 -std=c++11 main/main.cpp single/RubberBandSingle.cpp -o test_single -lsndfile
    
    echo " *** Running build from single-file source"
    ./test_single -V
    
    echo " *** OK"
    
else

    echo " *** Building static library using macOS-specific Makefile"
    make -f otherbuilds/Makefile.macos clean
    make -f otherbuilds/Makefile.macos

    echo " *** Linking against static library"
    c++ -std=c++11 main/main.cpp lib/librubberband.a -I. -Isrc -o test_static -I/opt/homebrew/include -L/opt/homebrew/lib -lsndfile -framework Accelerate
    
    echo " *** Running build from macOS-specific Makefile"
    ./test_static -V

    echo " *** Building with single-file source"
    c++ -O3 -std=c++11 main/main.cpp single/RubberBandSingle.cpp -o test_single -I/opt/homebrew/include -L/opt/homebrew/lib -lsndfile -framework Accelerate

    echo " *** Running build from single-file source"
    ./test_single -V

    echo " *** Building static library using iOS-specific Makefile"
    make -f otherbuilds/Makefile.ios clean
    make -f otherbuilds/Makefile.ios
    
fi
