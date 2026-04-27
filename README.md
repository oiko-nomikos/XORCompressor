# XOR Compressor

A C++ program that compresses text using a combination of word-level indexing and XOR-based bit compression.

---

## How It Works

The compression pipeline runs in two stages:

**Stage 1 — Word Indexing**
The input text is tokenized (preserving whitespace and punctuation as tokens). Each unique word is assigned a numeric ID, and a symbol table is built mapping each word ID to its positions in the text. This index is then serialized into a compact binary byte stream.

**Stage 2 — XOR Compression**
The binary byte stream is converted to a binary string, padded to the next power of 2, then compressed using a pairwise XOR scheme:
- Bits are processed in pairs `(A, B)`
- The **key** stores bit `A`
- The **XOR value** stores `A XOR B`
- To reconstruct `B`: `B = Key XOR XOR_Value`

This halves the size of the binary representation. The result is written to a file (`compressed.txt`).

**Decompression** reverses the process exactly: load the file → XOR decompress → binary to bytes → decode symbol table → reconstruct original text.

---

## Project Structure

```
├── main.cpp           # Full source — all classes and entry point
├── LICENSE            # Proprietary licence — all rights reserved
└── compressed.txt     # Output file produced after compression
```

### Classes

| Class | Responsibility |
|---|---|
| `XORCompress` | Core compression/decompression logic and word indexing |
| `Functions` | Utility helpers (byte↔binary conversion, power-of-2 padding) |
| `FileSystem` | Generic file read/write |
| `UserInterface` | Interactive CLI — runs the compress/decompress pipeline |

---

## Building

Requires a C++17-compatible compiler.

**Linux / macOS**
```bash
g++ -std=c++17 -O2 -o xor_compressor main.cpp
```

**Windows (MSVC)**
```bash
cl /std:c++17 /O2 main.cpp /Fe:xor_compressor.exe
```

**Windows (MinGW)**
```bash
g++ -std=c++17 -O2 -o xor_compressor.exe main.cpp
```

---

## Usage

Run the executable and follow the prompts:

```
Welcome to the Program...

Press Enter to continue...

ORIGINAL:
<text is displayed>

Compress data? (y/n): y

--- COMPRESSED ---
Key size: 1234
XOR size: 1234

Decompress data? (y/n): y

Reading file...
Restored binary size: 2468

--- RESTORED TEXT ---
<original text is displayed>
```

The compressed output is saved to `compressed.txt` in the working directory.

---

## Output File Format

`compressed.txt` uses a simple plain-text format:

```
==================================================
                 XOR COMPRESSOR
==================================================

KEY
<binary key string>

==================================================
XOR VALUE
<binary XOR string>

==================================================
```

---

## XOR Schema Reference

```
TWO ROWS:       Row A   01000000
                Row B   00100000
                XOR   = 01100000

ONE ROW:  {A,B} = {0,0},{1,0},{0,1},{0,0},...

SECOND ORDER XOR TABLE:
  Bit Order | XOR Value | Derived Key | Order
  ----------+-----------+-------------+------------
     00      |     0     |      0      | {B, A}
     10      |     1     |      1      | {A, B}
     01      |     1     |      0      | {B, A}
     11      |     0     |      1      | {A, B}
```

---

## Limitations

- Word IDs and positions are stored as `uint16_t`, so the compressor supports up to **65,535 unique words** and **65,535 total tokens**.
- The input text is currently hardcoded in `UserInterface::getInputText()`. Swap this out to read from a file or `stdin` for general use.
- XOR compression is most effective on data with low entropy (many repeated patterns). It is not a general-purpose compression algorithm.

---

## License

Copyright © 2026 Oiko Nomikos. All Rights Reserved.

This software is proprietary. **No use, copying, modification, or distribution is permitted without the explicit written permission of the author.** See [`LICENSE`](LICENSE) for full terms.
