#FROM ubuntu:20.04
FROM ubuntu:focal
ENV DEBIAN_FRONTEND=noninteractive

ARG CODE_DIR=/usr/local/src

RUN apt update

#basic environment
RUN apt install -y \
    ca-certificates \
    build-essential \
    git \
    cmake \
    cmake-curses-gui \
    libace-dev \
    libassimp-dev \
    libglew-dev \
    libglfw3-dev \
    libglm-dev \
    libeigen3-dev \
    clang-format

# Suggested dependencies for YARP
RUN apt update && apt install -y \
    qtbase5-dev qtdeclarative5-dev qtmultimedia5-dev \
    qml-module-qtquick2 qml-module-qtquick-window2 \
    qml-module-qtmultimedia qml-module-qtquick-dialogs \
    qml-module-qtquick-controls qml-module-qt-labs-folderlistmodel \
    qml-module-qt-labs-settings \
    libqcustomplot-dev \
    libgraphviz-dev \
    libjpeg-dev \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-libav

# Add metavision-sdk in sources.list
RUN echo "deb [arch=amd64 trusted=yes] https://apt.prophesee.ai/dists/public/b4b3528d/ubuntu focal sdk" >> /etc/apt/sources.list &&\
    apt update

RUN apt install -y \
    libcanberra-gtk-module \
    mesa-utils \
    ffmpeg \
    libboost-program-options-dev \
    libopencv-dev \
    metavision-sdk

#my favourites
RUN apt install -y \
    vim \
    gdb \
    libpython3-dev \
    python3-dev

# Build Open Image Debugger
ARG OID_VERSION=v1.17.30
RUN cd $CODE_DIR && \
    git clone --depth 1 --branch $OID_VERSION https://github.com/OpenImageDebugger/OpenImageDebugger.git && \
    cd OpenImageDebugger && \
    git submodule init && \
    git submodule update && \
    cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local && \
    cmake --build build --config Release --target install -j 4

# Build Google test
ARG GTEST_VERSION=v1.15.0
RUN cd $CODE_DIR && \
    git clone --depth 1 --branch $GTEST_VERSION https://github.com/google/googletest.git && \
    cd googletest && \
    mkdir build && cd build && \
    cmake .. && \
    make -j `nproc` install

# YCM
ARG YCM_VERSION=v0.15.2
RUN cd $CODE_DIR && \
    git clone --depth 1 --branch $YCM_VERSION https://github.com/robotology/ycm.git && \
    cd ycm && \
    mkdir build && cd build && \
    cmake .. && \
    make -j `nproc` install


# YARP
ARG YARP_VERSION=v3.8.0
RUN cd $CODE_DIR && \
    git clone --depth 1 --branch $YARP_VERSION https://github.com/robotology/yarp.git &&\
    cd yarp &&\
    mkdir build && cd build &&\
    cmake .. &&\
    make -j `nproc` install

EXPOSE 10000/tcp 10000/udp
RUN yarp check


# event-driven
ARG ED_VERSION=main
RUN cd $CODE_DIR &&\
    git clone --depth 1 --branch $ED_VERSION https://github.com/robotology/event-driven.git &&\
    cd event-driven &&\
    mkdir build && cd build &&\
    cmake .. &&\
    make -j `nproc` install

# Add User ID and Group ID
ARG UNAME=ledge
ARG UID=1000
ARG GID=1000
RUN groupadd -g $GID -o $UNAME
RUN useradd -m -u $UID -g $GID -o -s /bin/bash $UNAME

# Add User into sudoers, can run sudo command without password
RUN apt update && apt install -y sudo
RUN usermod -aG sudo ${UNAME}
RUN echo "${UNAME} ALL=(ALL) NOPASSWD:ALL" | tee /etc/sudoers.d/${UNAME}

# Copy codes
COPY ./ /app/LEDGE

# Change owner of some folders for the development
RUN chown -R $UNAME:$UNAME $CODE_DIR /app

USER $UNAME
WORKDIR /home/${UNAME}

# Add Open Image Debugger to non-root user
RUN touch ~/.gdbinit && \
    echo "source /usr/local/OpenImageDebugger/oid.py" > ~/.gdbinit

# Install uv
RUN curl -LsSf https://astral.sh/uv/install.sh | sh

# Create python virtual environment via uv
ENV PATH="/home/ledge/.local/bin:$PATH"
RUN cd /app/LEDGE && \
    uv sync
