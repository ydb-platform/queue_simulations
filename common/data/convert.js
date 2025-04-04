/**
 * BMFont Binary Format Converter
 * Copyright (c) 2024 Arctic Engine Development Team
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

const fs = require('fs');

// Read the JSON font file
const fontData = JSON.parse(fs.readFileSync('JetBrainsMono-Regular.json', 'utf8'));

// BMFont binary format constants
const BLOCK_TYPE_INFO = 1;
const BLOCK_TYPE_COMMON = 2;
const BLOCK_TYPE_PAGES = 3;
const BLOCK_TYPE_CHARS = 4;
const BLOCK_TYPE_KERNING = 5;

// Helper functions
function writeInt8(buffer, value, offset) {
    buffer.writeInt8(value, offset);
    return offset + 1;
}

function writeUInt8(buffer, value, offset) {
    buffer.writeUInt8(value, offset);
    return offset + 1;
}

function writeInt16(buffer, value, offset) {
    buffer.writeInt16LE(value, offset);
    return offset + 2;
}

function writeUInt16(buffer, value, offset) {
    buffer.writeUInt16LE(value, offset);
    return offset + 2;
}

function writeInt32(buffer, value, offset) {
    buffer.writeInt32LE(value, offset);
    return offset + 4;
}

function writeUInt32(buffer, value, offset) {
    buffer.writeUInt32LE(value, offset);
    return offset + 4;
}

function writeString(buffer, str, offset) {
    buffer.write(str, offset);
    offset += str.length;
    return writeUInt8(buffer, 0, offset); // null terminator
}

// Calculate block sizes
const infoBlockSize = 14 + fontData.info.face.length + 1;
const commonBlockSize = 15;
const pagesBlockSize = fontData.pages.reduce((acc, page) => acc + 'JetBrainsMono_0.tga'.length + 1, 0);
const charsBlockSize = fontData.chars.length * 20;
const kerningBlockSize = fontData.kernings ? fontData.kernings.length * 10 : 0;

// Calculate total size
const totalSize = 4 + // BMF header
    (5 + infoBlockSize) + // Info block header + data
    (5 + commonBlockSize) + // Common block header + data
    (5 + pagesBlockSize) + // Pages block header + data
    (5 + charsBlockSize) + // Chars block header + data
    (kerningBlockSize > 0 ? 5 + kerningBlockSize : 0); // Optional kerning block

console.log('Total buffer size needed:', totalSize);

// Create buffer
const buffer = Buffer.alloc(totalSize);
let offset = 0;

// Write BMF header
offset = writeInt8(buffer, 66, offset);  // 'B'
offset = writeInt8(buffer, 77, offset);  // 'M'
offset = writeInt8(buffer, 70, offset);  // 'F'
offset = writeInt8(buffer, 3, offset);   // version

// Write INFO block
offset = writeUInt8(buffer, BLOCK_TYPE_INFO, offset);
offset = writeInt32(buffer, infoBlockSize, offset);
offset = writeInt16(buffer, fontData.info.size, offset);
offset = writeUInt8(buffer, fontData.info.smooth, offset);
offset = writeUInt8(buffer, fontData.info.unicode, offset);
offset = writeUInt16(buffer, fontData.info.stretchH, offset);
offset = writeUInt8(buffer, fontData.info.aa, offset);
offset = writeUInt8(buffer, fontData.info.padding[0], offset);
offset = writeUInt8(buffer, fontData.info.padding[1], offset);
offset = writeUInt8(buffer, fontData.info.padding[2], offset);
offset = writeUInt8(buffer, fontData.info.padding[3], offset);
offset = writeUInt8(buffer, fontData.info.spacing[0], offset);
offset = writeUInt8(buffer, fontData.info.spacing[1], offset);
offset = writeUInt8(buffer, fontData.info.outline, offset);
offset = writeString(buffer, fontData.info.face, offset);

// Write COMMON block
offset = writeUInt8(buffer, BLOCK_TYPE_COMMON, offset);
offset = writeInt32(buffer, commonBlockSize, offset);
offset = writeUInt16(buffer, fontData.common.lineHeight, offset);
offset = writeUInt16(buffer, fontData.common.base, offset);
offset = writeUInt16(buffer, fontData.common.scaleW, offset);
offset = writeUInt16(buffer, fontData.common.scaleH, offset);
offset = writeUInt16(buffer, fontData.pages.length, offset);
offset = writeUInt8(buffer, fontData.common.packed ? 1 : 0, offset);
offset = writeUInt8(buffer, fontData.common.alphaChnl, offset);
offset = writeUInt8(buffer, fontData.common.redChnl, offset);
offset = writeUInt8(buffer, fontData.common.greenChnl, offset);
offset = writeUInt8(buffer, fontData.common.blueChnl, offset);

// Write PAGES block
offset = writeUInt8(buffer, BLOCK_TYPE_PAGES, offset);
offset = writeInt32(buffer, pagesBlockSize, offset);
for (const page of fontData.pages) {
    offset = writeString(buffer, 'JetBrainsMono_0.tga', offset);
}

// Write CHARS block
offset = writeUInt8(buffer, BLOCK_TYPE_CHARS, offset);
offset = writeInt32(buffer, charsBlockSize, offset);
for (const char of fontData.chars) {
    offset = writeUInt32(buffer, char.id, offset);
    offset = writeUInt16(buffer, char.x, offset);
    offset = writeUInt16(buffer, char.y, offset);
    offset = writeUInt16(buffer, char.width, offset);
    offset = writeUInt16(buffer, char.height, offset);
    offset = writeInt16(buffer, char.xoffset, offset);
    offset = writeInt16(buffer, char.yoffset, offset);
    offset = writeInt16(buffer, char.xadvance, offset);
    offset = writeUInt8(buffer, char.page, offset);
    offset = writeUInt8(buffer, char.chnl || 1, offset);
}

// Write KERNING block if exists
if (fontData.kernings && fontData.kernings.length > 0) {
    offset = writeUInt8(buffer, BLOCK_TYPE_KERNING, offset);
    offset = writeInt32(buffer, kerningBlockSize, offset);
    for (const kerning of fontData.kernings) {
        offset = writeUInt32(buffer, kerning.first, offset);
        offset = writeUInt32(buffer, kerning.second, offset);
        offset = writeInt16(buffer, kerning.amount, offset);
    }
}

console.log('Final offset:', offset);
console.log('Buffer size:', buffer.length);

// Write the binary file
fs.writeFileSync('JetBrainsMono.fnt', buffer); 