# Set prefix directory for ttf2png executable
if [ -f "../../cmake-build-debug/ttf2png" ]; then
    TTF2PNG_PREFIX="../../cmake-build-debug"
else
    TTF2PNG_PREFIX="."
fi

${TTF2PNG_PREFIX}/ttf2png "-chars:0123456789ABCDEFIJKLMNOPQRSTUVWXYZ!=+-_?<>[]{}()*&%\$#@abcdefijklmnopqrstuvwxyz\\/" -width:8 -height:16 -char_width:8 -char_height:8 ./PixeloidMono.ttf ./PixeloidMono.png
${TTF2PNG_PREFIX}/ttf2png "-chars:0123456789ABCDEFIJKLMNOPQRSTUVWXYZ!=+-_?<>[]{}()*&%\$#@abcdefijklmnopqrstuvwxyz\\/" -width:8 -height:16 -char_width:8 -char_height:8 ./PixelOperatorMonoHB8.ttf ./PixelOperatorMonoHB8.png
${TTF2PNG_PREFIX}/ttf2png "-chars:0123456789ABCDEFIJKLMNOPQRSTUVWXYZ!=+-_?<>[]{}()*&%\$#@abcdefijklmnopqrstuvwxyz\\/" -width:8 -height:16 -char_width:8 -char_height:8 ./Ithaca-LVB75.ttf ./Ithaca-LVB75.png
${TTF2PNG_PREFIX}/ttf2png "-chars:0123456789ABCDEFIJKLMNOPQRSTUVWXYZ!=+-_?<>[]{}()*&%\$#@abcdefijklmnopqrstuvwxyz\\/" -width:8 -height:16 -char_width:8 -char_height:8 ./ConsolaMono-Book.ttf ./ConsolaMono-Book.png
${TTF2PNG_PREFIX}/ttf2png "-chars:0123456789ABCDEFIJKLMNOPQRSTUVWXYZ!=+-_?<>[]{}()*&%\$#@abcdefijklmnopqrstuvwxyz\\/" -width:8 -height:16 -char_width:8 -char_height:8 ./PixelifySans-Regular.ttf ./PixelifySans-Regular.png
${TTF2PNG_PREFIX}/ttf2png "-chars:0123456789ABCDEFIJKLMNOPQRSTUVWXYZ!=+-_?<>[]{}()*&%\$#@abcdefijklmnopqrstuvwxyz\\/" -width:8 -height:16 -char_width:8 -char_height:8 ./PressStart2P-Regular.ttf ./PressStart2P-Regular.png
${TTF2PNG_PREFIX}/ttf2png "-chars:0123456789ABCDEFIJKLMNOPQRSTUVWXYZ!=+-_?<>[]{}()*&%\$#@abcdefijklmnopqrstuvwxyz\\/" -width:8 -height:16 -char_width:8 -char_height:8 ./Tiny5-Regular.ttf ./Tiny5-Regular.png
${TTF2PNG_PREFIX}/ttf2png "-chars:0123456789ABCDEFIJKLMNOPQRSTUVWXYZ!=+-_?<>[]{}()*&%\$#@abcdefijklmnopqrstuvwxyz\\/" -width:8 -height:16 -char_width:8 -char_height:8 ./VT323-Regular.ttf ./VT323-Regular.png
