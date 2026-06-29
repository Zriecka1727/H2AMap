# HOI4 to AoH2 Map Converter

**A Windows C\+\+ Win32 map conversion tool for HOI4 → AoH2 modding**

[中文文档](https://github.com/Zriecka1727/H2AMap/blob/main/.github/README_zhCN.md)

## English

**HOI4 to AoH2 Map Converter** is a lightweight, standalone Windows desktop tool built in pure C\+\+ Win32 API\. It converts official *Hearts of Iron IV* province map data \(`provinces.bmp` \+ `definition.csv`\) into standard polygon contour coordinates for *Age of History 2* map modding\.

The project is fully configured for**CLion \+ CMake**, no Visual Studio required\.

### Features

- Auto\-detect multi\-language UI: EN / 简体中文 / 繁體中文 / 日本語 / Русский

- Full Per\-Monitor DPI V2 support for high\-resolution Windows displays

- Auto scan land, sea, islands and enclaves from province bitmap

- Topology\-based border classification: land border, coastline, outer map edge, undefined border

- Smart coordinate simplification protection to avoid terrain distortion

- Real\-time 4\-stage progress display with time statistics

- Auto fill undefined color blocks to prevent map loss

- Non\-blocking UI during conversion

### Build \(CLion\)

1. Open project root in CLion

2. CMake will automatically configure the project

3. Switch to **Release** mode

4. Build and get executable from `cmake-build-release/bin`

### Usage

1. Place `definition.csv` and `provinces.bmp` in the program folder

2. Set simplify factor \(1 = highest precision\)

3. Start conversion

4. Obtain final `MAP_POINTS.txt` for AoH2 mod

### Notes

- Only supports BMP bitmap input

- CSV file must use semicolon `;` separator

- Only converts geometric map contour data

- High\-resolution maps require more memory and time

---

**License:** MIT \| **Author:** Fanillanus
