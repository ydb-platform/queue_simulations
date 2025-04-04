# Font Conversion Instructions

This document describes how to convert JetBrains Mono font from TTF to the BMFont format used by Arctic Engine.

## Prerequisites

1. Install Node.js (v16 or later)
2. Install msdf-bmfont-xml globally:
   ```bash
   npm install -g msdf-bmfont-xml
   ```
3. Install ImageMagick for image format conversion:
   ```bash
   # macOS
   brew install imagemagick
   # Ubuntu/Debian
   sudo apt-get install imagemagick
   ```

## Original Font

Download JetBrains Mono Regular from the official repository:
https://github.com/JetBrains/JetBrainsMono/releases

Direct link to TTF:
https://github.com/JetBrains/JetBrainsMono/raw/master/fonts/ttf/JetBrainsMono-Regular.ttf

## Conversion Steps

1. Place the TTF file in the `common/data` directory:
   ```bash
   cd common/data
   ```

2. Generate BMFont JSON and texture:
   ```bash
   msdf-bmfont -f json \
     --font-size 24 \
     -m 256,256 \
     --pot \
     --smart-size \
     --texture-size 256,256 \
     --charset 32-126,160-255 \
     --field-type sdf \
     --distance-range 2 \
     JetBrainsMono-Regular.ttf
   ```
   This will create:
   - `JetBrainsMono-Regular.json` - Font metrics and layout
   - `JetBrainsMono-Regular.png` - Font texture atlas

3. Convert PNG texture to TGA:
   ```bash
   convert JetBrainsMono-Regular.png JetBrainsMono-Regular.tga
   ```

4. Convert JSON to binary BMFont format:
   ```bash
   node convert.js
   ```
   This will create:
   - `JetBrainsMono.fnt` - Binary BMFont file

## Output Files

The following files are needed for the engine:
- `JetBrainsMono.fnt` - Binary font file
- `JetBrainsMono-Regular.tga` - Font texture atlas

## Parameters Explanation

- `--font-size 24` - Font size in pixels
- `-m 256,256` - Maximum texture dimensions
- `--pot` - Make texture dimensions power of two
- `--smart-size` - Automatically adjust texture size
- `--texture-size 256,256` - Target texture size
- `--charset 32-126,160-255` - ASCII and extended Latin characters
- `--field-type sdf` - Signed Distance Field for sharp scaling
- `--distance-range 2` - SDF edge range in pixels 