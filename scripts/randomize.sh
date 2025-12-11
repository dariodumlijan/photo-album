#!/bin/bash

# Script to randomize .jpg filenames in assets/target
# This creates a random order for photo frame display

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Get the script's directory and project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Define target directory
TARGET_DIR="$PROJECT_ROOT/assets/target"

echo "Photo Frame Image Randomizer"
echo "============================="
echo ""

# Check if target directory exists
if [ ! -d "$TARGET_DIR" ]; then
    echo -e "${RED}Error: Target directory not found: $TARGET_DIR${NC}"
    exit 1
fi

# Find all .jpg files (case insensitive), excluding .keep file
shopt -s nullglob nocaseglob
JPG_FILES=("$TARGET_DIR"/*.jpg)
shopt -u nullglob nocaseglob

# Check if any jpg files were found
if [ ${#JPG_FILES[@]} -eq 0 ]; then
    echo -e "${YELLOW}No .jpg files found in $TARGET_DIR${NC}"
    echo "Please run prepare.sh first to generate images."
    exit 0
fi

echo "Found ${#JPG_FILES[@]} .jpg file(s) to randomize"
echo ""

# Create a temporary directory for renaming
TEMP_DIR="$TARGET_DIR/.temp_randomize_$$"
mkdir -p "$TEMP_DIR"

# Move all jpg files to temp directory with random names
echo "Randomizing filenames..."
COUNTER=1

for img in "${JPG_FILES[@]}"; do
    # Generate a random name using timestamp and random number
    random_name="img_$(date +%s%N)_${RANDOM}_${COUNTER}.jpg"
    
    # Move to temp directory
    mv "$img" "$TEMP_DIR/$random_name"
    
    ((COUNTER++))
done

# Now move them back with sequential names to preserve the random order
COUNTER=1
for img in "$TEMP_DIR"/*.jpg; do
    # Pad counter with zeros (e.g., 001, 002, etc.)
    padded_counter=$(printf "%03d" $COUNTER)
    new_name="${padded_counter}.jpg"
    
    mv "$img" "$TARGET_DIR/$new_name"
    echo -e "  ${GREEN}✓${NC} Created: $new_name"
    
    ((COUNTER++))
done

# Remove temporary directory
rmdir "$TEMP_DIR"

echo ""
echo "============================="
echo -e "${GREEN}Successfully randomized ${#JPG_FILES[@]} file(s)!${NC}"
echo ""
echo "Files have been renamed with random order in: $TARGET_DIR"
