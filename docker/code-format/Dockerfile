#
# Build a Docker image with clang-format
#
# Update the container image and container registry by running:
# -  From this directory:
#       $ ./../build_and_push_docker_images.sh
#
# -  From the docker/ directory:
#       $ ./build_and_push_docker_images.sh
#
# -  From the holiday-widget directory:
#       $ ./docker/build_and_push_docker_images.sh
#
# Note: This will build and push all docker images
#

FROM ubuntu AS build

# Install dependencies
RUN apt-get update && \
    apt-get install -y git cmake build-essential python3

# Clone and build the latest 18.x LLVM version
RUN git clone --branch release/18.x --depth=1 https://github.com/llvm/llvm-project.git
RUN cmake -B build -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=Release llvm-project/llvm
RUN make -j$(nproc) -C build clang-format
RUN make -C build install-clang-format

FROM ubuntu

RUN apt-get update && \
    apt-get install -y git python3-minimal

COPY --from=build /usr/local /usr/local
