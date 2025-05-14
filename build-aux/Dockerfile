# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

FROM ubuntu:20.04

# Set non-interactive frontend (needed to bypass prompts during apt install)
ENV DEBIAN_FRONTEND=noninteractive

# Basic dependencies
RUN apt-get update && apt-get install -y \
    build-essential wget vim zlib1g-dev \
    git pkg-config xdg-utils libxml2-utils flex gettext \
    libdw-dev libjack-jackd2-dev help2man libasound2-dev libfreetype6-dev \
    python3 python3-pip libgl1-mesa-dev libx11-dev libxext-dev libxrender-dev \
    libxrandr-dev libxi-dev libxcursor-dev libxcomposite-dev libxdamage-dev \
    libxfixes-dev libxinerama-dev libxkbcommon-dev libwayland-dev \
    libegl1-mesa-dev libdrm-dev libgbm-dev ladspa-sdk \
    libxcb-glx0-dev libxcb-icccm4-dev \
    libxcb-image0-dev libxcb-keysyms1 libxcb-keysyms1-dev \
    libxcb-shm0-dev libxcb-util-dev libxcb-util0-dev libxcb-util1 \
    libxcb-xkb-dev libxcb-xkb1 pax-utils libfontconfig1-dev itstool \
    libxcb-cursor-dev libxcb-randr0-dev libxcb-render-util0-dev libxcb1-dev \
    libxcb-xinerama0-dev libxcb-shape0-dev libxcb-sync-dev libxcb-xfixes0-dev \
    libxkbcommon-x11-dev

# GCC
RUN wget https://gcc.gnu.org/pub/gcc/releases/gcc-15.1.0/gcc-15.1.0.tar.xz && \
    tar -xf gcc-15.1.0.tar.xz && \
    cd gcc-15.1.0 && \
    ./contrib/download_prerequisites && \
    mkdir gcc-build && cd gcc-build && \
    ../configure CFLAGS_FOR_TARGET='-O2 -fPIC -mtune=generic -fstack-protector-strong -D_FORTIFY_SOURCE=2' \
    CXXFLAGS_FOR_TARGET='-O2 -fPIC -mtune=generic -fstack-protector-strong -D_FORTIFY_SOURCE=2' \
    --prefix=/usr/local --enable-languages=c,c++,lto \
    --disable-multilib --with-build-config=bootstrap-lto \
    --with-linker-hash-style=gnu --with-system-zlib \
    --enable-__cxa_atexit --enable-cet=auto --enable-checking=release \
    --enable-clocale=gnu --enable-default-pie --enable-default-ssp \
    --enable-gnu-indirect-function --enable-gnu-unique-object \
    --enable-libstdcxx-backtrace --enable-link-serialization=1 \
    --enable-linker-build-id --enable-lto --enable-plugin --enable-shared \
    --enable-threads=posix --disable-libssp --disable-libstdcxx-pch \
    --disable-werror && \
    make -j$(nproc) && \
    make install && \
    cd .. && \
    rm -rf gcc-15.1.0.tar.xz gcc-15.1.0 && \
    gcc --version && g++ --version

# Valgrind
RUN wget https://sourceware.org/pub/valgrind/valgrind-3.25.0.tar.bz2 && \
    tar -xf valgrind-3.25.0.tar.bz2 && \
    cd valgrind-3.25.0 && \
    ./configure --prefix=/usr/local --enable-lto && \
    make -j$(nproc) && \
    make install && \
    cd .. && \
    rm -rf valgrind-3.25.0.tar.bz2 valgrind-3.25.0 && \
    valgrind --version

# Build tools
RUN pip3 install meson cmake ninja reuse

# Boost
RUN wget https://archives.boost.io/release/1.88.0/source/boost_1_88_0.tar.gz && \
    tar -xf boost_1_88_0.tar.gz && \
    cd boost_1_88_0 && \
    ./bootstrap.sh && \
    ./b2 && ./b2 install --prefix=/usr/local && \
    cd .. && \
    rm -rf boost_1_88_0.tar.gz boost_1_88_0

# LV2
RUN wget https://lv2plug.in/spec/lv2-1.18.10.tar.xz && \
    tar -xf lv2-1.18.10.tar.xz && \
    cd lv2-1.18.10 && \
    meson setup build --prefix /usr/local && \
    meson compile -C build && \
    meson install -C build && \
    cd .. && \
    rm -rf lv2-1.18.10.tar.xz lv2-1.18.10

# Build environment (FIXME: CPATH/LIBRARY_PATH might not be needed)
ENV PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/local/lib64/pkgconfig:$PKG_CONFIG_PATH \
    PATH=/usr/local/bin:$PATH \
    LIBRARY_PATH=/usr/local/lib:/usr/local/lib64:$LIBRARY_PATH \
    CPATH=/usr/local/include:$CPATH \
    LD_LIBRARY_PATH=/usr/local/lib:/usr/local/lib64:$LD_LIBRARY_PATH \
    CMAKE_PREFIX_PATH=/usr/local:$CMAKE_PREFIX_PATH \
    QT_QPA_PLATFORM=offscreen \
    BOOST_ROOT=/usr/local

# Qt
RUN wget https://download.qt.io/official_releases/qt/6.9/6.9.0/single/qt-everywhere-src-6.9.0.tar.xz && \
    tar -xf qt-everywhere-src-6.9.0.tar.xz && \
    cd qt-everywhere-src-6.9.0 && \
    mkdir build-qt-release && cd build-qt-release && \
    ../configure -prefix /usr/local -release -no-gtk -qt-zlib -qt-libjpeg \
    -qt-libpng -qt-freetype -qt-pcre -qt-harfbuzz -opensource \
    -confirm-license -skip qtgrpc -skip qtdoc -skip qtwebengine \
    -xcb -unity-build -pch -force-bundled-libs && \
    cmake --build . --parallel && \
    cmake --install . && \
    cd ../.. && \
    rm -rf qt-everywhere-src-6.9.0.tar.xz qt-everywhere-src-6.9.0 && \
    qmllint --version

# Install gitlab runner too so container can be used as a runner
# (needs manual registration)
RUN curl -L "https://packages.gitlab.com/install/repositories/runner/gitlab-runner/script.deb.sh" | bash && \
    apt-get install -y gitlab-runner && \
    gitlab-runner --version

# Install various utilities and clean up to reduce image size
RUN apt-get install -y curl sudo \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Create a non-root user for runtime
RUN useradd -m appuser

# Switch to non-root user
USER appuser

# Set working directory
WORKDIR /home/appuser