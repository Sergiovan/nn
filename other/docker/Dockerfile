FROM ubuntu:plucky
SHELL ["/bin/bash", "-c"]

# Install dependencies
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt install \
    nodejs \
    build-essential \
    g++-15 \
    clang \
    cmake \
    pkg-config \
    libglib2.0-dev \
    git \
    flex \
    bison \
    ninja-build \
    gcc-riscv64-unknown-elf \
    llvm \
    wget \
    python3.13 \
    python3.13-venv \
    python-is-python3 \
    pip \
    -y

# This is a hack but it's fine, we're not going to be using these forever
RUN ln -s /usr/bin/riscv64-unknown-elf-as /usr/bin/riscv64-elf-as
RUN ln -s /usr/bin/riscv64-unknown-elf-ld /usr/bin/riscv64-elf-ld
RUN ln -s /usr/bin/riscv64-unknown-elf-objcopy /usr/bin/riscv64-elf-objcopy

# Install qemu-riscv with all the dependencies
RUN wget https://download.qemu.org/qemu-10.0.0.tar.xz
RUN tar xJf qemu-10.0.0.tar.xz
WORKDIR /qemu-10.0.0 
RUN ./configure --target-list=riscv64-linux-user,riscv64-softmmu
RUN make -j
RUN make install -j

# run by mounting with -v PATH_TO_REPO:/sirgovan-rust/
WORKDIR /home/root/ 

ENTRYPOINT ["tail", "-f", "/dev/null"]