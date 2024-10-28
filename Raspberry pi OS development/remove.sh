#!/bin/bash

# Define potential directories
DIR1="/run/media/diego/bootfs"
DIR2="/run/media/diego/bootfs1"

# Function to remove kernel8.img if it exists
remove_kernel_image() {
  if [ -f "$1/kernel8.img" ]; then
    echo "Removing $1/kernel8.img"
    rm "$1/kernel8.img"
    echo "Removed $1/kernel8.img"
  else
    echo "No kernel8.img found in $1"
  fi
}

# Check and remove kernel8.img from both directories
remove_kernel_image "$DIR1"
remove_kernel_image "$DIR2"
