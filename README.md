# NES Tools - PNG to CHR and TTF to PNG Converters

A collection of command-line utilities NES system development.

## Overview

**nes_tools** provides two complementary tools for NES asset creation:

- **png2chr**: Converts PNG images to NES CHR (character) format
- **ttf2png**: Renders TrueType fonts to PNG grids compatible with NES tools

## Tools

### png2chr

Converts indexed-color PNG images into NES CHR (CHRacter) format, used for sprite and background tile data.

#### Features

- Converts 8x8 pixel tiles to NES 2-bitplane CHR format
- Supports palette-based PNG images
- Validates input dimensions (must be multiples of 8x8)
- Handles 2-bit color depth (4 colors per tile)
- Comprehensive error reporting

#### Usage

```bash
png2chr <input.png> <output.chr>
```

#### Requirements

- **Input PNG Format**:
    - Must be indexed color (palette-based)
    - Dimensions must be multiples of 8x8 pixels
    - Palette colors: 0-3 (2 bits per pixel)
    - Each 8x8 tile is converted to 16 bytes in CHR format

#### Example

```bash
png2chr tileset.png output.chr
```

---

### ttf2png

Renders characters from a TrueType font into a PNG grid, creating font bitmaps suitable for NES applications.

#### Features

- Renders individual characters from TTF fonts to a customizable PNG grid
- Supports configurable character dimensions
- Creates 8-bit indexed color PNG output
- Flexible grid layout (width × height)
- Handles various font files with proper glyph rendering

#### Usage

```bash
ttf2png -chars:<characters> -width:<cols> -height:<rows> [-char_width:<w>] [-char_height:<h>] <input.ttf> <output.png>
```

#### Arguments

| Argument              | Description                           | Default |
|-----------------------|---------------------------------------|---------|
| `-chars:<characters>` | Characters to render (required)       | -       |
| `-width:<cols>`       | Grid width (number of columns)        | -       |
| `-height:<rows>`      | Grid height (number of rows)          | -       |
| `-char_width:<w>`     | Character pixel width                 | 8       |
| `-char_height:<h>`    | Character pixel height                | 8       |
| `<input.ttf>`         | Path to TrueType font file (required) | -       |
| `<output.png>`        | Path to output PNG file (required)    | -       |

#### Output Format

- **PNG Type**: Indexed color, 8-bit
- **Palette**: 2-color (black and white)
- **Grid Size**: \`width × height\` characters
- **Image Size**: \`(width × char_width) × (height × char_height)\` pixels

#### Example

```bash
ttf2png -chars:ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -width:8 -height:5 -char_width:8 -char_height:16 font.ttf font_output.png
```

This creates a 8×5 grid (40 characters) with 8×16 pixel characters, resulting in an 64×80 pixel PNG.

---

## Building

The project uses CMake for building. Ensure you have the required dependencies installed:

### Dependencies

- **libpng**: PNG image manipulation
- **FreeType2**: Font rasterization (ttf2png only)
- **C99 or later**: Required for compilation

### Build Instructions

```bash
mkdir build
cd build
cmake ..
make
```

Executables will be created in the \`build/\` directory.

---

## Workflow: Font to CHR

A typical workflow for creating NES fonts:

1. **Render TTF to PNG**:
   ```bash
   ttf2png -chars:ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 -width:8 -height:5 font.ttf font.png
   ```

2. **Convert PNG to CHR**:
   ```bash
   png2chr font.png font.chr
   ```

---

## Test Assets

The project includes sample fonts in \`data_test/ttf2png/\`:
- PressStart2P-Regular.ttf
- PixelifySans-Regular.ttf
- VT323-Regular.ttf
- PixeloidMono.ttf
- Ithaca-LVB75.ttf
- And more...

Sample PNG for testing png2chr is located in \`data_test/png2chr/\`.

---