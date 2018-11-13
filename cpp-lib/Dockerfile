FROM ubuntu:18.04

RUN apt-get update && apt-get --assume-yes install golang cmake g++-7 libboost-locale-dev libcpprest-dev git libgtest-dev

ADD . cpp-lib
RUN cd cpp-lib && \
    rm -rf _builds/linux-make-release && \
    mkdir -p _builds/linux-make-release && \
    cmake -B_builds/linux-make-release -H. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu -DBUILD_TESTING=OFF && \
    cmake --build _builds/linux-make-release

