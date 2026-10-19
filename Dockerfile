FROM ghcr.io/epicgames/unreal-engine:dev-5.7.4

ARG SUMO_REF=v1_27_1

RUN sudo apt update \
&& sudo apt install git cmake libxerces-c-dev zlib1g-dev -y \
&& python3 -m pip install --user swig==4.3.1 \
&& git clone --branch "$SUMO_REF" --depth 1 https://github.com/eclipse-sumo/sumo.git \
&& cd sumo \
&& export TOOLCHAIN=/home/ue4/UnrealEngine/Engine/Extras/ThirdPartyNotUE/SDKs/HostLinux/Linux_x64/v26_clang-20.1.8-rockylinux8/x86_64-unknown-linux-gnu \
&& export PATH="/home/ue4/.local/bin:$PATH" \
&& cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DCMAKE_C_COMPILER=$TOOLCHAIN/bin/clang \
  -DCMAKE_CXX_COMPILER=$TOOLCHAIN/bin/clang++ \
  -DCMAKE_CXX_FLAGS="-stdlib=libc++ -fPIC" \
  -DCMAKE_EXE_LINKER_FLAGS="-stdlib=libc++" \
  -DCMAKE_SHARED_LINKER_FLAGS="-stdlib=libc++" \
  -DENABLE_PYTHON_BINDINGS=OFF \
  -DENABLE_JAVA_BINDINGS=OFF \
  -DENABLE_CS_BINDINGS=OFF \
  -DENABLE_C_BINDINGS=ON \
&& cmake --build build --target libsumoc -j$(nproc)
