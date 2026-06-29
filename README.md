HOI4 to AoH2 Map Converter

Convert Hearts of Iron IV province map to Age of History 2 map coordinate data \(Windows C\+\+ Win32 Tool\)

\[中文文档\]\(\#中文说明\)

## Introduction

A native Windows desktop tool written in C\+\+ Win32 API, used to convert HOI4 `provinces.bmp` \+ `definition.csv` into AoH2 readable `MAP_POINTS.txt` contour coordinate file\.This program automatically recognizes provinces, islands, enclaves, shared borders, coastlines and map outer edges, supports configurable point simplification to reduce output file size\.

### Features

* Multi\-language UI: Auto detect system language \| EN / 简体中文 / 繁體中文 / 日本語 / Русский

* Per\-Monitor DPI Aware V2, auto scale UI \& font for high\-DPI Windows displays

* Full automatic raster scan: Identify all land/sea provinces, standalone islands \& enclaves

* Topology\-based shared border extraction, classify: land border / coastline / map outer border / unregistered color boundary

* Smart point simplification rules:
  
  * Map outer edges \& unregistered regions: no simplification \(fixed factor = 1\)
  
  * Coastline maximum simplify factor limited to 2 to avoid terrain distortion
  
  * Custom global simplify factor \(minimum = 1\)

* 4\-stage progress display with real\-time percentage, elapsed time statistics

* Auto assign temporary virtual ID for undefined colors in `definition.csv` to prevent missing map blocks

* Single\-thread background conversion, non\-blocking UI

## Requirements

### Input Files \(Place in same folder with EXE\)

1. `definition.csv` — HOI4 official province definition file \(semicolon `;` separated\)

2. `provinces.bmp` — HOI4 province color bitmap image

### Build Dependencies

* Windows SDK \(Win32 \+ Common Controls\)

* Single\-header library: `stb_image.h` \(included in project\)

* Link library: `Comctl32.lib`

* Compiler: MSVC \(Visual Studio 2019/2022 recommended\)

## Build Guide

1. Create an empty C\+\+ Windows Desktop Project in Visual Studio

2. Set project configuration:
   
   * Character Set: Use Unicode Character Set
   
   * DPI Awareness: Per Monitor High DPI Aware V2

3. Add source file with provided code, put `stb_image.h` in source directory

4. Linker → Input → Additional Dependencies: add `Comctl32.lib`

5. Build as Release x64 / x86

## Usage

1. Put `definition.csv` and `provinces.bmp` in the same directory as compiled `.exe`

2. Launch the converter

3. Set **Simplify** value \(minimum 1, larger value = smaller output file\)

4. Click `Start Convert`

5. Conversion runs in 4 steps:
   
   1. Scan all provinces, islands and enclaves
   
   2. Extract shared topological borders
   
   3. Simplify contour coordinate points
   
   4. Assemble closed polygon loops for each province

6. After finished, `MAP_POINTS.txt` will be generated in current folder

7. Use this file directly in your Age of History 2 mod map

### Simplify Parameter Rules

* `1`: No simplification, highest precision, largest file size

* `>1`: Keep one coordinate point every N points

* Hard constraints:
  
  * Outer map border \& unregistered color regions: always factor = 1
  
  * Coastline border: maximum allowed factor = 2

## Output Format: `MAP_POINTS.txt`

Each province occupies exactly two lines:

1. Line 1: Comma\-separated X coordinates of closed contour

2. Line 2: Comma\-separated Y coordinates of closed contour

Empty/invalid province outputs two lines of `0`\.Unregistered color blocks will be appended sequentially after original registered provinces\.

## Limitations

* Only supports BMP bitmap input

* CSV must use semicolon `;` as separator \(HOI4 standard format\)

* Only converts geometry contour data; no population, terrain, culture or province name conversion

* Large high\-resolution maps require more RAM and longer conversion time

## Author

Author: FanillanusQQ: 3156785429
