#!/bin/bash

if [ -f "../../cmake-build-debug/chr2png" ]; then
    CHR2PNG_PREFIX="../../cmake-build-debug"
else
    CHR2PNG_PREFIX="."
fi

# Test current directory
${CHR2PNG_PREFIX}/chr2png -i -c ./title.chr -n ./title.nametable -r 32 -o title.png

# Execute png2chr and generate png again
cd ../png2chr
sh ../png2chr/png2chr_test.sh

cd ../chr2png
${CHR2PNG_PREFIX}/chr2png -i -c ../png2chr/title_1.chr -n ../png2chr/title_1.nametable -r 32 -o ./title_1.png
${CHR2PNG_PREFIX}/chr2png -i -c ../png2chr/title_2.chr -n ../png2chr/title_2.nametable -r 32 -o ./title_2.png

# -- In this case original title.png required more than 256 tiles
# -- Expected CHR must crop the original title picture
# -- Check if it's the case after conversion from CHR to PNG
${CHR2PNG_PREFIX}/chr2png -i -c ../png2chr/title_3.chr -r 32 -o ./title_3.png

# -- If we use same -r 16 as original command for the CHR
# -- then PNG output file will be NOT rendered correctly because CHR don't crop original PNG file rows
${CHR2PNG_PREFIX}/chr2png -i -c ../png2chr/title_4.chr -r 16 -o ./title_4.png
# -- If we use -r 32 then the output PNG file will be rendered properly
${CHR2PNG_PREFIX}/chr2png -i -c ../png2chr/title_4.chr -r 32 -o ./title_4-1.png