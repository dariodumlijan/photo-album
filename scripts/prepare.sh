#!/bin/bash

# Script to resize images from assets/root to assets/target
# Target resolution: 480x320 (keeping aspect ratio)

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Get the script's directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Define source and target directories
SOURCE_DIR="$PROJECT_ROOT/assets/root"
TARGET_DIR="$PROJECT_ROOT/assets/target"

# Target resolution
TARGET_WIDTH=480
TARGET_HEIGHT=320
TARGET_RESOLUTION="${TARGET_WIDTH}x${TARGET_HEIGHT}"

echo "Photo Frame Image Preparation Script"
echo "====================================="
echo ""

# Check if magick command exists
if ! command -v magick &> /dev/null; then
    echo -e "${RED}Error: ImageMagick 'magick' command not found.${NC}"
    echo "Please install ImageMagick to use this script."
    echo ""
    echo "Installation instructions:"
    echo "  macOS:   brew install imagemagick"
    echo "  Ubuntu:  sudo apt-get install imagemagick"
    echo "  Windows: Download from https://imagemagick.org/script/download.php"
    exit 1
fi

echo -e "${GREEN}✓ ImageMagick found${NC}"
echo ""

# Check if source directory exists
if [ ! -d "$SOURCE_DIR" ]; then
    echo -e "${RED}Error: Source directory not found: $SOURCE_DIR${NC}"
    exit 1
fi

# Create target directory if it doesn't exist
if [ ! -d "$TARGET_DIR" ]; then
    echo -e "${YELLOW}Creating target directory: $TARGET_DIR${NC}"
    mkdir -p "$TARGET_DIR"
fi

# Find all common image files (case insensitive)
# Supports: jpg, jpeg, png, gif, bmp, tiff, tif, webp, heic, heif
shopt -s nullglob nocaseglob
IMAGE_FILES=("$SOURCE_DIR"/*.jpg "$SOURCE_DIR"/*.jpeg "$SOURCE_DIR"/*.png \
             "$SOURCE_DIR"/*.gif "$SOURCE_DIR"/*.bmp "$SOURCE_DIR"/*.tiff \
             "$SOURCE_DIR"/*.tif "$SOURCE_DIR"/*.webp "$SOURCE_DIR"/*.heic \
             "$SOURCE_DIR"/*.heif)
shopt -u nullglob nocaseglob

# Check if any images were found
if [ ${#IMAGE_FILES[@]} -eq 0 ]; then
    echo -e "${YELLOW}No image files found in $SOURCE_DIR${NC}"
    echo "Supported formats: jpg, jpeg, png, gif, bmp, tiff, tif, webp, heic, heif"
    echo "Please add some images to the assets/root directory."
    exit 0
fi

echo "Found ${#IMAGE_FILES[@]} image(s) to process"
echo "Target resolution: ${TARGET_RESOLUTION} (aspect ratio preserved)"
echo ""

# Process each image
PROCESSED=0
FAILED=0

for img in "${IMAGE_FILES[@]}"; do
    # Get the filename without path and extension
    filename=$(basename "$img")
    filename_no_ext="${filename%.*}"
    
    # Always save as .jpg
    target_path="$TARGET_DIR/${filename_no_ext}.jpg"
    
    echo -n "Processing: $filename ... "
    
    # Resize image keeping aspect ratio, fitting within 480x320
    # Convert to JPG format with quality 90
    # The '>' flag only shrinks images larger than the target size
    if magick "$img" -resize "${TARGET_RESOLUTION}>" -quality 90 "$target_path" 2>/dev/null; then
        echo -e "${GREEN}✓ Done${NC}"
        ((PROCESSED++))
    else
        echo -e "${RED}✗ Failed${NC}"
        ((FAILED++))
    fi
done

echo ""
echo "====================================="
echo "Processing complete!"
echo -e "${GREEN}Successfully processed: $PROCESSED${NC}"
if [ $FAILED -gt 0 ]; then
    echo -e "${RED}Failed: $FAILED${NC}"
fi
echo ""
echo "Resized images saved to: $TARGET_DIR"