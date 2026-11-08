# SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

FROM ubuntu:22.04

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
    libxkbcommon-x11-dev python3.10-venv ccache libc6-dbg libdbus-1-dev libfuse2 \
    libx11-xcb-dev libfontenc-dev libice-dev libsm-dev libxaw7-dev libxkbfile-dev \
    libxmu-dev libxmuu-dev libxpm-dev libxres-dev libxss-dev libxt-dev libxtst-dev \
    libxv-dev libxxf86vm-dev libxcb-dri3-dev libxcb-dri2-0-dev libxcb-present-dev \
    libxcb-composite0-dev libxcb-ewmh-dev libxcb-res0-dev

# Binutils
RUN wget https://sourceware.org/pub/binutils/releases/binutils-2.46.0.tar.xz && \
    tar -xf binutils-2.46.0.tar.xz && \
    cd binutils-2.46.0 && \
    mkdir build && cd build && \
    ../configure --prefix=/usr/local \
    --enable-plugins --enable-lto \
    --with-system-zlib && \
    make -j$(nproc) && \
    make install && \
    cd ../.. && \
    rm -rf binutils-2.46.0.tar.xz binutils-2.46.0 && \
    ld --version

# GCC
RUN wget https://gcc.gnu.org/pub/gcc/releases/gcc-16.1.0/gcc-16.1.0.tar.xz && \
    tar -xf gcc-16.1.0.tar.xz && \
    cd gcc-16.1.0 && \
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
    --disable-werror --with-ld=/usr/local/bin/ld && \
    make -j$(nproc) && \
    make install && \
    cd .. && \
    rm -rf gcc-16.1.0.tar.xz gcc-16.1.0 && \
    update-alternatives --install /usr/bin/gcc gcc /usr/local/bin/gcc 100 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/local/bin/g++ 100 && \
    update-alternatives --install /usr/bin/cc cc /usr/local/bin/gcc 100 && \
    update-alternatives --install /usr/bin/c++ c++ /usr/local/bin/g++ 100 && \
    cc --version && c++ --version && gcc --version && g++ --version


# Build environment (FIXME: CPATH/LIBRARY_PATH might not be needed)
ENV PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/local/lib64/pkgconfig:$PKG_CONFIG_PATH \
    PATH=/usr/local/bin:$PATH \
    LIBRARY_PATH=/usr/local/lib:/usr/local/lib64:$LIBRARY_PATH \
    CPATH=/usr/local/include:$CPATH \
    LD_LIBRARY_PATH=/usr/local/lib:/usr/local/lib64:$LD_LIBRARY_PATH \
    CMAKE_PREFIX_PATH=/usr/local:$CMAKE_PREFIX_PATH \
    QT_QPA_PLATFORM=offscreen \
    APPIMAGE_EXTRACT_AND_RUN=1

# Set up environment for all users
RUN echo 'export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/local/lib64/pkgconfig:$PKG_CONFIG_PATH' > /etc/profile.d/zrythm_env.sh && \
    echo 'export PATH=/usr/local/bin:$PATH' >> /etc/profile.d/zrythm_env.sh && \
    echo 'export LIBRARY_PATH=/usr/local/lib:/usr/local/lib64:$LIBRARY_PATH' >> /etc/profile.d/zrythm_env.sh && \
    echo 'export CPATH=/usr/local/include:$CPATH' >> /etc/profile.d/zrythm_env.sh && \
    echo 'export LD_LIBRARY_PATH=/usr/local/lib:/usr/local/lib64:$LD_LIBRARY_PATH' >> /etc/profile.d/zrythm_env.sh && \
    echo 'export CMAKE_PREFIX_PATH=/usr/local:$CMAKE_PREFIX_PATH' >> /etc/profile.d/zrythm_env.sh && \
    echo 'export QT_QPA_PLATFORM=offscreen' >> /etc/profile.d/zrythm_env.sh && \
    chmod +x /etc/profile.d/zrythm_env.sh

# Expose system wayland .pc files to Conan builds (wayland.enable_libraries=False
# means Conan only builds wayland-scanner; the actual libraries come from the system)
RUN mkdir -p /usr/local/lib/pkgconfig && \
    ln -sf /usr/lib/x86_64-linux-gnu/pkgconfig/wayland-client.pc /usr/local/lib/pkgconfig/ && \
    ln -sf /usr/lib/x86_64-linux-gnu/pkgconfig/wayland-server.pc /usr/local/lib/pkgconfig/ && \
    ln -sf /usr/lib/x86_64-linux-gnu/pkgconfig/wayland-cursor.pc /usr/local/lib/pkgconfig/ && \
    ln -sf /usr/lib/x86_64-linux-gnu/pkgconfig/wayland-egl.pc /usr/local/lib/pkgconfig/

# Valgrind
RUN wget https://sourceware.org/pub/valgrind/valgrind-3.25.1.tar.bz2 && \
    tar -xf valgrind-3.25.1.tar.bz2 && \
    cd valgrind-3.25.1 && \
    ./configure --prefix=/usr/local --enable-lto && \
    make -j$(nproc) && \
    make install && \
    cd .. && \
    rm -rf valgrind-3.25.1.tar.bz2 valgrind-3.25.1 && \
    valgrind --version

# Python 3.12 (from deadsnakes PPA — Sphinx 9.x requires Python 3.11+)
RUN apt-get install -y software-properties-common && \
    add-apt-repository -y ppa:deadsnakes/ppa && \
    apt-get update && \
    apt-get install -y python3.12 python3.12-venv python3.12-dev && \
    update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.12 2 && \
    update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.10 1 && \
    python3 -m ensurepip --upgrade && \
    python3 -m pip install --upgrade pip && \
    python3 --version

# Build tools
RUN python3 -m pip install meson cmake ninja reuse

# Install various utilities
RUN apt-get install -y curl sudo patchelf

# appimagetool (not available in Ubuntu repos)
RUN curl -L -o /usr/local/bin/appimagetool \
    "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage" && \
    chmod +x /usr/local/bin/appimagetool && \
    appimagetool --version

# sccache (shared compiler cache; S3 backend configured at runtime via env)
RUN SCCACHE_VERSION=0.16.0 && \
    wget -qO- "https://github.com/mozilla/sccache/releases/download/v${SCCACHE_VERSION}/sccache-v${SCCACHE_VERSION}-x86_64-unknown-linux-musl.tar.gz" | \
    tar xz -C /tmp && \
    install -m755 "/tmp/sccache-v${SCCACHE_VERSION}-x86_64-unknown-linux-musl/sccache" /usr/local/bin && \
    rm -rf /tmp/sccache-v${SCCACHE_VERSION}-x86_64-unknown-linux-musl && \
    sccache --version

# Install gitlab runner too so container can be used as a runner
# (needs manual registration)
RUN curl -L "https://packages.gitlab.com/install/repositories/runner/gitlab-runner/script.deb.sh" | bash && \
    apt-get install -y gitlab-runner && \
    gitlab-runner --version

# Clang 23 (from apt.llvm.org) — used by clang-based builds, sanitizers,
# clazy, clang-format, clang-tidy
RUN wget -qO- https://apt.llvm.org/llvm-snapshot.gpg.key | \
      tee /etc/apt/trusted.gpg.d/apt.llvm.org.asc && \
    echo "deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-23 main" \
      > /etc/apt/sources.list.d/llvm.list && \
    apt-get update && \
    apt-get install -y \
      clang-23 clang-tools-23 clang-format-23 clang-tidy-23 \
      libclang-rt-23-dev libclang-23-dev libc++-23-dev libc++abi-23-dev && \
    update-alternatives --install /usr/bin/clang clang /usr/bin/clang-23 100 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-23 100 && \
    update-alternatives --install /usr/bin/clang-format clang-format /usr/bin/clang-format-23 100 && \
    update-alternatives --install /usr/bin/clang-tidy clang-tidy /usr/bin/clang-tidy-23 100 && \
    ln -sf /usr/share/clang/clang-tidy-diff-23.py /usr/share/clang/clang-tidy-diff.py && \
    clang --version

# mold linker — used by configure-for-tools job (-DCMAKE_LINKER_TYPE=MOLD)
RUN MOLD_VERSION=2.41.0 && \
    wget -qO- "https://github.com/rui314/mold/releases/download/v${MOLD_VERSION}/mold-${MOLD_VERSION}-x86_64-linux.tar.gz" | \
      tar xz -C /tmp && \
    cp -a /tmp/mold-${MOLD_VERSION}-x86_64-linux/bin/mold /usr/local/bin/ && \
    ln -sf mold /usr/local/bin/ld.mold && \
    cp -a /tmp/mold-${MOLD_VERSION}-x86_64-linux/lib/mold /usr/local/lib/ && \
    rm -rf /tmp/mold-${MOLD_VERSION}-x86_64-linux && \
    mold --version

# Documentation, manual building & utility packages
RUN apt-get install -y \
      doxygen graphviz rsync zip libimage-exiftool-perl

# Dart Sass — required by user manual build (find_program(SASS_EXECUTABLE "sass"))
RUN SASS_VERSION=1.101.0 && \
    wget -qO- "https://github.com/sass/dart-sass/releases/download/${SASS_VERSION}/dart-sass-${SASS_VERSION}-linux-x64.tar.gz" | \
      tar xz -C /opt && \
    ln -sf /opt/dart-sass/sass /usr/local/bin/sass && \
    sass --version

# TeX Live (full, all languages) — for PDF manual generation
RUN apt-get install -y \
      texlive-latex-extra texlive-fonts-recommended texlive-lang-all

# Clean up to reduce image size
RUN apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Create a non-root user for runtime
RUN useradd -m appuser

# Switch to non-root user
USER appuser

# Set working directory
WORKDIR /home/appuser

# To build Zrythm:
# conan install . -pr:h gcc_debug -pr:b gcc_release --build=missing
# cmake --preset default
# cmake --build conanbuild/Debug --parallel
# QT_QPA_PLATFORM=offscreen conanbuild/Debug/products/bin/zrythm --version
