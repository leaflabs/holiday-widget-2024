#!/bin/bash

# Set the working directory to holiday-widget
# (the parent directory of this script's directory)
cd "$(dirname "$0")/.."

# Define color codes
RED='\033[1;31m'   # Bold Red
GREEN='\033[1;32m' # Bold Green
NC='\033[0m'       # No Color
BOLD_NC='\033[1m'  # Bold No Color

# Function to build and push a Docker image
build_and_push() {
    local dockerfile=$1
    local image_tag=$2

    echo -e "${BOLD_NC}Building Docker image for $dockerfile...${NC}"

    # Build the Docker image
    if docker buildx build -f "$dockerfile" -t "$image_tag" .; then
        echo -e "${GREEN}Successfully built $image_tag${NC}"
        
        # Push the Docker image
        echo -e "${BOLD_NC}Pushing Docker image $image_tag...${NC}"
        if docker push "$image_tag"; then
            echo -e "${GREEN}Successfully pushed $image_tag${NC}"
        else
            echo -e "${RED}Failed to push $image_tag${NC}"
            exit 1
        fi
    else
        echo -e "${RED}Failed to build $image_tag${NC}"
        exit 1
    fi
}

# Define Dockerfile paths and image tags
CODE_FORMAT_DOCKERFILE="docker/code-format/Dockerfile"
CODE_FORMAT_IMAGE_TAG="registry.gitlab.leaflabs.com/internal-projects/holiday-widget/code-format"

BUILD_FIRMWARE_DOCKERFILE="docker/build-firmware/Dockerfile"
BUILD_FIRMWARE_IMAGE_TAG="registry.gitlab.leaflabs.com/internal-projects/holiday-widget/build-firmware"

# Build and push the Docker images
build_and_push "$CODE_FORMAT_DOCKERFILE" "$CODE_FORMAT_IMAGE_TAG"
build_and_push "$BUILD_FIRMWARE_DOCKERFILE" "$BUILD_FIRMWARE_IMAGE_TAG"
