# Set prefix directory for ttf2png executable
if [ -f "../../cmake-build-debug/ttf2png" ]; then
    TTF2PNG_PREFIX="../../cmake-build-debug"
else
    TTF2PNG_PREFIX="."
fi

${TTF2PNG_PREFIX}/ttf2png "-chars: !\"#\$%&'()*+,-./:;<=>?@[\\]^_{|}~0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefighjklmnopqrstuvwxyz" -width:16 -height:7 -char_width:8 -char_height:8 ./PixeloidMono.ttf ./PixeloidMono.png
${TTF2PNG_PREFIX}/ttf2png "-chars: !\"#\$%&'()*+,-./:;<=>?@[\\]^_{|}~0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefighjklmnopqrstuvwxyz" -width:16 -height:7 -char_width:8 -char_height:8 ./PixelOperatorMonoHB8.ttf ./PixelOperatorMonoHB8.png
${TTF2PNG_PREFIX}/ttf2png "-chars: !\"#\$%&'()*+,-./:;<=>?@[\\]^_{|}~0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefighjklmnopqrstuvwxyz" -width:16 -height:7 -char_width:8 -char_height:8 ./Ithaca-LVB75.ttf ./Ithaca-LVB75.png
${TTF2PNG_PREFIX}/ttf2png "-chars: !\"#\$%&'()*+,-./:;<=>?@[\\]^_{|}~0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefighjklmnopqrstuvwxyz" -width:16 -height:7 -char_width:8 -char_height:8 ./ConsolaMono-Book.ttf ./ConsolaMono-Book.png
${TTF2PNG_PREFIX}/ttf2png "-chars: !\"#\$%&'()*+,-./:;<=>?@[\\]^_{|}~0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefighjklmnopqrstuvwxyz" -width:16 -height:7 -char_width:8 -char_height:8 ./PixelifySans-Regular.ttf ./PixelifySans-Regular.png
${TTF2PNG_PREFIX}/ttf2png "-chars: !\"#\$%&'()*+,-./:;<=>?@[\\]^_{|}~0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz" -width:16 -height:7 -char_width:8 -char_height:8 ./PressStart2P-Regular.ttf ./PressStart2P-Regular.png
${TTF2PNG_PREFIX}/ttf2png "-chars: !\"#\$%&'()*+,-./:;<=>?@[\\]^_{|}~0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz" -width:16 -height:7 -char_width:8 -char_height:8 ./Tiny5-Regular.ttf ./Tiny5-Regular.png
${TTF2PNG_PREFIX}/ttf2png "-chars: !\"#\$%&'()*+,-./:;<=>?@[\\]^_{|}~0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz" -width:16 -height:7 -char_width:8 -char_height:8 ./VT323-Regular.ttf ./VT323-Regular.png


# CHR Mapping :
#    if (c == ' ') return 0;
#    if (c >= '!' && c <= '@') return (c - 32); // Ponctuation et @
#    if (c >= '0' && c <= '9') return (c + 16); // Chiffres
#    if (c >= 'A' && c <= 'Z') return (c - 23); // Majuscules
#    if (c >= 'a' && c <= 'z') return (c - 29); // Minuscules
#    if (c >= '[' && c <= '_') return (c - 60); // Symboles [ \ ] ^ _
#    if (c >= '{' && c <= '~') return (c - 86); // Symboles { | } ~

