#!/bin/bash

# Pipeline script to prepare and randomize images
# This script combines prepare.sh and randomize.sh into one workflow

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Get the script's directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Photo Frame Pipeline"
echo "===================="
echo ""
echo "This pipeline will:"
echo "  1. Prepare images (resize from assets/root to assets/target)"
echo "  2. Randomize image filenames"
echo ""

# Step 1: Run prepare.sh
echo -e "${YELLOW}Step 1: Running prepare.sh...${NC}"
echo ""

if bash "$SCRIPT_DIR/prepare.sh"; then
    echo ""
    echo -e "${GREEN}✓ Prepare step completed successfully${NC}"
    echo ""
else
    echo ""
    echo -e "${RED}✗ Prepare step failed${NC}"
    echo "Pipeline aborted."
    exit 1
fi

# Step 2: Run randomize.sh
echo -e "${YELLOW}Step 2: Running randomize.sh...${NC}"
echo ""

if bash "$SCRIPT_DIR/randomize.sh"; then
    echo ""
    echo -e "${GREEN}✓ Randomize step completed successfully${NC}"
    echo ""
else
    echo ""
    echo -e "${RED}✗ Randomize step failed${NC}"
    echo "Pipeline aborted."
    exit 1
fi

# Pipeline complete
echo "===================="
echo -e "${GREEN}Pipeline completed successfully!${NC}"
echo ""
echo "Your images are ready in assets/target with randomized order."
