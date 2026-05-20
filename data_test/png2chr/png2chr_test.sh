#!/bin/bash

if [ -f "../../cmake-build-debug/png2chr" ]; then
    PNG2CHR_PREFIX="../../cmake-build-debug"
else
    PNG2CHR_PREFIX="."
fi

# 1- Generate smaller CHR with nametable, CHR with 32 tiles by row
${PNG2CHR_PREFIX}/png2chr -i ./title.png -o ./title_1.chr -n ./title_1.nametable -r 32 -f false -e auto

# 2- Generate standard CHR with nametable, fill the end of the file to reach 4k, CHR with 32 tiles by row
${PNG2CHR_PREFIX}/png2chr -i ./title.png -o ./title_2.chr -n ./title_2.nametable -r 32 -f true -e auto

# 3- Generate CHR without nametable, CHR with 32 tiles by row
# -- In this case our title.png require more than 256 tiles limitation
# -- Expected CHR must crop the original title picture
${PNG2CHR_PREFIX}/png2chr -i ./title.png -o ./title_3.chr -r 32 -f false -e auto

# 4- Generate CHR without nametable, CHR with 16 tiles by row, fix empty tile index to 0
${PNG2CHR_PREFIX}/png2chr -i ./title.png -o ./title_4.chr -r 16 -f false -e 0

# 4- Generate CHR without nametable, CHR with 16 tiles by row, fix empty tile index to 0
${PNG2CHR_PREFIX}/png2chr -i ./characters_8x16.png -o ./characters_8x16.chr -r 16 -f false