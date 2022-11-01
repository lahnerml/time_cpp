FROM archlinux:latest

USER root
WORKDIR /root

# Update system, install C++
RUN pacman -Syu --noconfirm
RUN pacman -S --noconfirm cmake gcc gdb git make

# Install date library
RUN git clone https://github.com/HowardHinnant/date.git && \
    cd "date" && \
    mkdir -p build && \
    cd "build" && \
    cmake .. \
        -DBUILD_TZ_LIB=ON \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr && \
    make install -j 8 && make clean

COPY docker-entrypoint.sh /

RUN useradd build

USER build
WORKDIR /home/build

# Install rust
RUN curl --proto '=https' --tlsv1.3 -sSf https://sh.rustup.rs -o rust.sh && \
    chmod +x rust.sh && \
    ./rust.sh -y

# Fetch software

ENTRYPOINT [ "/docker-entrypoint.sh" ]