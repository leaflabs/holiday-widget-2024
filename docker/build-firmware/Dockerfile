#
# Build a Docker image with the tools needed to build the firmware
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
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    git \
    python3 \
    python3-pip \
    gcc-arm-none-eabi \
    libnewlib-arm-none-eabi \
    gdb-multiarch \
    build-essential \
    make \
    libgl1

# Copy files excluded from the .dockerignore to local
COPY . .

# Install Python packages from requirements.txt
RUN python3 -m pip install -r ./firmware/scripts/requirements.txt --break-system-packages
