FROM conanio/clang14-ubuntu18.04
RUN sudo apt-get update && sudo apt-get --assume-yes install golang-go ninja-build
RUN pip3 install --upgrade conan cmake
RUN conan profile detect
RUN mkdir ~/.conan2/p
