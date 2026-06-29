#include <windows.h>
#include <commctrl.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <future>
#include <sstream>
#include <chrono>
#include <thread>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <queue>

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#pragma comment(lib, "Comctl32.lib")

// 基礎數據結構
struct MapPoint {
    int x; int y;
    bool operator==(const MapPoint& o) const { return x == o.x && y == o.y; }
    bool operator!=(const MapPoint& o) const { return x != o.x || y != o.y; }
};

struct PointHash {
    size_t operator()(const MapPoint& p) const {
        return ((long long)p.x << 32) | (unsigned int)p.y;
    }
};

struct ProvDef { int id; bool isSea; bool isUnregistered; };

// 全局唯一的公共邊界
struct TopologyEdge {
    size_t edgeId;
    int provA;
    int provB;
    bool isUnregisteredBoundary;
    bool isCoastline;
    bool isMapOuterEdge;
    std::vector<MapPoint> originalPoints;
    std::vector<MapPoint> simplifiedPoints;
};

// 獨立島嶼/飛地實體
struct IslandEntity {
    size_t id;
    int parentProvId;
    int colorKey;
    bool isSea;
    bool isUnregistered;
    std::vector<std::vector<MapPoint>> finalLoops;
};

// 全局控制元件
HWND g_hwndMain;
HWND g_progressBar; HWND g_statusText; HWND g_timeText;
HWND g_editBox; HWND g_button; HWND g_closeButton; HWND g_helpButton; HFONT g_font;
std::atomic<int> completed(0); int simplifyAmount = 1;

// 多语言系统
enum Lang { EN, ZH_CN, ZH_TW, JA, RU };
Lang g_lang = EN;

struct LocaleStr {
    const wchar_t* ready;
    const wchar_t* tip_files;
    const wchar_t* simplify;
    const wchar_t* start;
    const wchar_t* close;
    const wchar_t* step1;
    const wchar_t* step2;
    const wchar_t* step3;
    const wchar_t* step4;
    const wchar_t* loading_csv;
    const wchar_t* loading_bmp;
    const wchar_t* error_bmp;
    const wchar_t* done;
    const wchar_t* exported;
    const wchar_t* help_title;
    const wchar_t* help_content;
    const wchar_t* window_title;
};

LocaleStr loc{};

void InitLanguage() {
    LANGID langId = GetUserDefaultUILanguage();
    WORD primary = langId & 0xFF;

    if (primary == 0x04) { // 中文
        if (langId == 0x0804) g_lang = ZH_CN;
        else if (langId == 0x0404) g_lang = ZH_TW;
        else g_lang = ZH_CN;
    }
    else if (primary == 0x11) g_lang = JA; // 日语
    else if (primary == 0x19) g_lang = RU; // 俄语
    else g_lang = EN;

    switch (g_lang) {
    case EN:
        loc = {
            L"Ready",
            L"Put definition.csv and provinces.bmp in current folder",
            L"Simplify:",
            L"Start Convert",
            L"Close",
            L"Step 1/4: Scan Provinces & Islands",
            L"Step 2/4: Extract Shared Borders",
            L"Step 3/4: Simplify Points",
            L"Step 4/4: Build Contours",
            L"Loading definition.csv...",
            L"Loading provinces.bmp...",
            L"Error: provinces.bmp not found",
            L"Completed! Time: %.1fs | Registered colors: %d",
            L"Map data exported (100%)",
            L"About",
            L"HOI4 to Age of History 2 Map Converter\n\n"
            L"1. Higher simplify = smaller file\n"
            L"2. Outer border protected\n"
            L"3. Coastline max simplify = 2\n\n"
            L"Author: Fanillanus | QQ: 3156785429",
            L"HOI4 to AoH2 Map Converter"
        };
        break;
    case ZH_CN:
        loc = {
            L"就绪",
            L"请将 definition.csv 和 provinces.bmp 放在当前目录",
            L"简化倍率:",
            L"开始转换",
            L"关闭",
            L"步骤 1/4: 识别省份与独立区块",
            L"步骤 2/4: 提取共享边界线",
            L"步骤 3/4: 简化边界点",
            L"步骤 4/4: 组装轮廓",
            L"正在读取 definition.csv...",
            L"正在加载 provinces.bmp...",
            L"错误：找不到 provinces.bmp 文件",
            L"完成！用时：%.1f 秒 | 自动填充色块：%d",
            L"地图数据导出完成 (100%)",
            L"使用说明",
            L"钢铁雄心4 转 历史时代2 地图转换器\n\n"
            L"1. 简化倍率越大，文件越小\n"
            L"2. 地图边缘自动保护\n"
            L"3. 海岸线简化最大为2\n\n"
            L"作者：Fanillanus | QQ：3156785429",
            L"钢铁雄心4 转 历史时代2 地图转换器"
        };
        break;
    case ZH_TW:
        loc = {
            L"就緒",
            L"請將 definition.csv 和 provinces.bmp 放在當前目錄",
            L"簡化倍率:",
            L"開始轉換",
            L"關閉",
            L"步驟 1/4: 識別省份與獨立區塊",
            L"步驟 2/4: 提取共享邊界線",
            L"步驟 3/4: 簡化邊界點",
            L"步驟 4/4: 組裝輪廓",
            L"正在讀取 definition.csv...",
            L"正在載入 provinces.bmp...",
            L"錯誤：找不到 provinces.bmp 檔案",
            L"完成！耗時：%.1f 秒 | 自動填充色塊：%d",
            L"地圖資料匯出完成 (100%)",
            L"使用說明",
            L"鋼鐵雄心4 轉 歷史時代2 地圖轉換器\n\n"
            L"1. 簡化倍率越大，檔案越小\n"
            L"2. 地圖邊緣自動保護\n"
            L"3. 海岸線簡化最大為2\n\n"
            L"作者：Fanillanus | QQ：3156785429",
            L"鋼鐵雄心4 轉 歷史時代2 地圖轉換器"
        };
        break;
    case JA:
        loc = {
            L"準備完了",
            L"definition.csv と provinces.bmp を同フォルダに入れてください",
            L"簡易化:",
            L"変換開始",
            L"閉じる",
            L"ステップ1/4: 地域をスキャン",
            L"ステップ2/4: 境界線抽出",
            L"ステップ3/4: ポイント簡略化",
            L"ステップ4/4: 輪郭作成",
            L"definition.csv 読込中...",
            L"provinces.bmp 読込中...",
            L"エラー: provinces.bmp が見つかりません",
            L"完了！時間：%.1f 秒 | 登録色数：%d",
            L"地図データ出力完了 (100%)",
            L"使い方",
            L"HOI4 → Age of History2 地図コンバーター\n\n"
            L"1. 数値が大きいほどファイルは小さくなります\n"
            L"2. 地図の外周は自動で保護されます\n"
            L"3. 海岸線の最大簡略化は2です\n\n"
            L"作者: Fanillanus | QQ: 3156785429",
            L"HOI4 → AoH2 地図コンバーター"
        };
        break;
    case RU:
        loc = {
            L"Готово",
            L"Положите definition.csv и provinces.bmp в папку с программой",
            L"Упрощение:",
            L"Начать конвертацию",
            L"Закрыть",
            L"Шаг 1/4: Сканирование провинций",
            L"Шаг 2/4: Извлечение границ",
            L"Шаг 3/4: Упрощение точек",
            L"Шаг 4/4: Сборка контуров",
            L"Загрузка definition.csv...",
            L"Загрузка provinces.bmp...",
            L"Ошибка: provinces.bmp не найден",
            L"Завершено! Время: %.1fс | Цвета: %d",
            L"Карта экспортирована (100%)",
            L"О программе",
            L"Конвертер карт HOI4 в AoH2\n\n"
            L"1. Чем больше упрощение, тем меньше файл\n"
            L"2. Внешние границы защищены\n"
            L"3. Побережье упрощается не более 2\n\n"
            L"Автор: Fanillanus | QQ: 3156785429",
            L"Конвертер карт HOI4 → AoH2"
        };
        break;
    }
}
std::chrono::steady_clock::time_point g_lastUpdateTime;

int GetWindowDpi(HWND hwnd) {
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        typedef UINT(WINAPI* GDFW)(HWND);
        auto pGDFW = (GDFW)GetProcAddress(hUser32, "GetDpiForWindow");
        if (pGDFW) return pGDFW(hwnd);
    }
    HDC hdc = GetDC(hwnd); int dpi = GetDeviceCaps(hdc, LOGPIXELSX); ReleaseDC(hwnd, hdc);
    return dpi > 0 ? dpi : 96;
}
int ScalePixels(int p, int dpi) { return MulDiv(p, dpi, 96); }

HFONT createUIFontForDpi(int dpi) {
    return CreateFontW(MulDiv(-15, dpi, 96), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei UI");
}

void applyFont(HWND hwnd) { SendMessageW(hwnd, WM_SETFONT, (WPARAM)g_font, TRUE); }
inline int rgbToKey(int r, int g, int b) { return (r << 16) | (g << 8) | b; }

void setText(HWND hwnd, const std::wstring& text) {
    SetWindowTextW(hwnd, text.c_str());
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
}

void updateSmoothProgress(const std::wstring& stage, int stageNum, int curr, int total) {
    if (curr > total) curr = total;
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - g_lastUpdateTime).count() >= 16 || curr == total) {
        g_lastUpdateTime = now;

        double stageProgress = (total > 0) ? (double)curr / total : 1.0;
        int globalPos = (stageNum - 1) * 2500 + static_cast<int>(stageProgress * 2500);

        SendMessage(g_progressBar, PBM_SETPOS, globalPos, 0);

        std::wstringstream ss;
        ss << L"" << stage.c_str() << L" | " << (int)(globalPos / 100.0) << L"% (" << curr << L"/" << total << L")";
        setText(g_statusText, ss.str());
    }
}

std::unordered_map<int, ProvDef> loadDefinitions(const std::string& path) {
    std::unordered_map<int, ProvDef> defs;
    std::ifstream file(path); std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line); std::string token; std::vector<std::string> parts;
        while (std::getline(ss, token, ';')) {
            token.erase(0, token.find_first_not_of(" \t\r\n"));
            token.erase(token.find_last_not_of(" \t\r\n") + 1);
            parts.push_back(token);
        }
        if (parts.size() < 5) continue;
        try {
            int id = std::stoi(parts[0]);
            int r = std::stoi(parts[1]); int g = std::stoi(parts[2]); int b = std::stoi(parts[3]);
            std::string type = parts[4];
            std::transform(type.begin(), type.end(), type.begin(), ::tolower);
            bool isSea = (type == "sea" || type == "lake" || type == "ocean");
            defs[rgbToKey(r, g, b)] = ProvDef{ id, isSea, false };
        } catch (...) { continue; }
    }
    return defs;
}

void runGenerator() {
    auto start = std::chrono::high_resolution_clock::now();
    g_lastUpdateTime = std::chrono::steady_clock::now();

    SendMessage(g_progressBar, PBM_SETRANGE32, 0, 10000);
    SendMessage(g_progressBar, PBM_SETPOS, 0, 0);

    setText(g_statusText, loc.loading_csv);
    auto defs = loadDefinitions("definition.csv");

    setText(g_statusText, loc.loading_bmp);
    int width, height, channels;
    unsigned char* image = stbi_load("provinces.bmp", &width, &height, &channels, 3);
    if (!image) { setText(g_statusText, loc.error_bmp); return; }

    std::vector<int> mapData(width * height); std::unordered_set<int> presentColors;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int i = (y * width + x) * 3;
            int key = rgbToKey(image[i], image[i + 1], image[i + 2]);
            mapData[y * width + x] = key; presentColors.insert(key);
        }
    }
    stbi_image_free(image);

    std::vector<int> unregisteredColors;
    for (int key : presentColors) {
        if (defs.find(key) == defs.end()) {
            unregisteredColors.push_back(key);
        }
    }
    std::sort(unregisteredColors.begin(), unregisteredColors.end(), [](int a, int b) {
        long long rA = (a >> 16) & 0xFF, gA = (a >> 8) & 0xFF, bA = a & 0xFF;
        long long rB = (b >> 16) & 0xFF, gB = (b >> 8) & 0xFF, bB = b & 0xFF;
        return (rA * gA * bA) > (rB * gB * bB);
    });

    std::unordered_map<int, int> provIdToColorKey;
    int maxRegisteredProvId = 0;
    for (const auto& pair : defs) {
        if (presentColors.count(pair.first)) {
            provIdToColorKey[pair.second.id] = pair.first;
            if (pair.second.id > maxRegisteredProvId) {
                maxRegisteredProvId = pair.second.id;
            }
        }
    }

    int tempVirtualID = 950000;
    int missingCount = (int)unregisteredColors.size();
    for (int key : unregisteredColors) {
        defs[key] = ProvDef{ tempVirtualID++, false, true };
    }

    std::vector<int> islandMap(width * height, -1);
    std::vector<IslandEntity> allIslands;
    std::vector<bool> visited(width * height, false);

    struct Pixel { int x; int y; };
    size_t islandCounter = 0;

    int scanTotal = height * width;
    int pixelsChecked = 0;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            pixelsChecked++;
            if (visited[y * width + x]) {
                if (pixelsChecked % 10000 == 0) {
                    updateSmoothProgress(loc.step1, 1, pixelsChecked, scanTotal);
                }
                continue;
            }
            int colorKey = mapData[y * width + x];
            auto def = defs[colorKey];

            std::queue<Pixel> q;
            q.push({ x, y });
            visited[y * width + x] = true;
            int currIslandId = (int)islandCounter++;

            IslandEntity isl;
            isl.id = currIslandId;
            isl.parentProvId = def.id;
            isl.colorKey = colorKey;
            isl.isSea = def.isSea;
            isl.isUnregistered = def.isUnregistered;

            while (!q.empty()) {
                Pixel curr = q.front(); q.pop();
                islandMap[curr.y * width + curr.x] = currIslandId;

                int dx[] = { 0, 1, 0, -1 }; int dy[] = { -1, 0, 1, 0 };
                for (int i = 0; i < 4; i++) {
                    int nx = curr.x + dx[i]; int ny = curr.y + dy[i];
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                        if (!visited[ny * width + nx] && mapData[ny * width + nx] == colorKey) {
                            visited[ny * width + nx] = true;
                            q.push({ nx, ny });
                        }
                    }
                }
            }
            allIslands.push_back(isl);
            if (pixelsChecked % 10000 == 0) {
                updateSmoothProgress(loc.step1, 1, pixelsChecked, scanTotal);
            }
        }
    }
    updateSmoothProgress(loc.step1, 1, scanTotal, scanTotal);

    auto makeIslandPairKey = [](int id1, int id2) {
        if (id1 > id2) std::swap(id1, id2);
        return ((long long)id1 << 32) | (unsigned int)id2;
    };

    struct GridEdge { MapPoint p1; MapPoint p2; int leftIsland; int rightIsland; };
    std::vector<GridEdge> rawGridEdges;

    auto getIslandAt = [&](int x, int y) {
        if (x < 0 || x >= width || y < 0 || y >= height) return -2;
        return islandMap[y * width + x];
    };

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int centerIsland = getIslandAt(x, y);
            int gx = x * 2, gy = y * 2;

            int northIsland = getIslandAt(x, y - 1);
            if (centerIsland != northIsland && centerIsland > northIsland) {
                rawGridEdges.push_back({ {gx, gy}, {gx + 2, gy}, centerIsland, northIsland });
            }
            int eastIsland = getIslandAt(x + 1, y);
            if (centerIsland != eastIsland && centerIsland > eastIsland) {
                rawGridEdges.push_back({ {gx + 2, gy}, {gx + 2, gy + 2}, centerIsland, eastIsland });
            }
            int southIsland = getIslandAt(x, y + 1);
            if (centerIsland != southIsland && centerIsland > southIsland) {
                rawGridEdges.push_back({ {gx + 2, gy + 2}, {gx, gy + 2}, centerIsland, southIsland });
            }
            int westIsland = getIslandAt(x - 1, y);
            if (centerIsland != westIsland && centerIsland > westIsland) {
                rawGridEdges.push_back({ {gx, gy + 2}, {gx, gy}, centerIsland, westIsland });
            }
        }
    }

    std::unordered_map<long long, std::vector<GridEdge>> pairToEdges;
    for (const auto& ge : rawGridEdges) {
        pairToEdges[makeIslandPairKey(ge.leftIsland, ge.rightIsland)].push_back(ge);
    }

    std::vector<TopologyEdge> globalTopologyEdges;
    size_t globalEdgeIdCounter = 0;
    int currentPairIdx = 0;
    int totalPairs = (int)pairToEdges.size();

    for (auto& pairItem : pairToEdges) {
        currentPairIdx++;
        long long pairKey = pairItem.first; auto& gEdges = pairItem.second;
        int id1 = (int)(pairKey >> 32); int id2 = (int)(pairKey & 0xFFFFFFFF);

        std::unordered_multimap<long long, size_t> ptToGeIdx;
        for (size_t i = 0; i < gEdges.size(); i++) {
            ptToGeIdx.insert({ ((long long)gEdges[i].p1.x << 32) | (unsigned int)gEdges[i].p1.y, i });
            ptToGeIdx.insert({ ((long long)gEdges[i].p2.x << 32) | (unsigned int)gEdges[i].p2.y, i });
        }

        std::vector<bool> geUsed(gEdges.size(), false);
        for (size_t startIdx = 0; startIdx < gEdges.size(); startIdx++) {
            if (geUsed[startIdx]) continue;

            std::vector<MapPoint> edgeChain;
            geUsed[startIdx] = true;
            edgeChain.push_back(gEdges[startIdx].p1);
            edgeChain.push_back(gEdges[startIdx].p2);

            while (true) {
                MapPoint head = edgeChain.front();
                long long headHash = ((long long)head.x << 32) | (unsigned int)head.y;
                auto range = ptToGeIdx.equal_range(headHash); size_t foundIdx = -1;
                for (auto it = range.first; it != range.second; ++it) {
                    if (!geUsed[it->second]) { foundIdx = it->second; break; }
                }
                if (foundIdx == -1) break;
                geUsed[foundIdx] = true;
                if (gEdges[foundIdx].p1 == head) edgeChain.insert(edgeChain.begin(), gEdges[foundIdx].p2);
                else edgeChain.insert(edgeChain.begin(), gEdges[foundIdx].p1);
            }

            while (true) {
                MapPoint tail = edgeChain.back();
                long long tailHash = ((long long)tail.x << 32) | (unsigned int)tail.y;
                auto range = ptToGeIdx.equal_range(tailHash); size_t foundIdx = -1;
                for (auto it = range.first; it != range.second; ++it) {
                    if (!geUsed[it->second]) { foundIdx = it->second; break; }
                }
                if (foundIdx == -1) break;
                geUsed[foundIdx] = true;
                if (gEdges[foundIdx].p1 == tail) edgeChain.push_back(gEdges[foundIdx].p2);
                else edgeChain.push_back(gEdges[foundIdx].p1);
            }

            TopologyEdge tEdge;
            tEdge.edgeId = globalEdgeIdCounter++;
            tEdge.provA = gEdges[startIdx].leftIsland;
            tEdge.provB = gEdges[startIdx].rightIsland;
            tEdge.originalPoints = edgeChain;

            bool hasUnregistered = false; bool hasLand = false; bool hasSea = false; bool hasMapOuter = false;
            auto checkIslandProps = [&](int islandId) {
                if (islandId == -2) { hasMapOuter = true; return; }
                if (allIslands[islandId].isUnregistered) hasUnregistered = true;
                if (allIslands[islandId].isSea) hasSea = true;
                else hasLand = true;
            };
            checkIslandProps(tEdge.provA); checkIslandProps(tEdge.provB);

            tEdge.isUnregisteredBoundary = hasUnregistered;
            tEdge.isMapOuterEdge = hasMapOuter;
            tEdge.isCoastline = (hasSea && hasLand && !hasMapOuter);

            globalTopologyEdges.push_back(tEdge);
        }
        if (currentPairIdx % 100 == 0 || currentPairIdx == totalPairs) {
            updateSmoothProgress(loc.step2, 2, currentPairIdx, totalPairs);
        }
    }
    updateSmoothProgress(loc.step2, 2, totalPairs, totalPairs);

    std::unordered_map<int, std::vector<size_t>> islandToEdgesMap;
    for (const auto& te : globalTopologyEdges) {
        if (te.provA >= 0) islandToEdgesMap[te.provA].push_back(te.edgeId);
        if (te.provB >= 0) islandToEdgesMap[te.provB].push_back(te.edgeId);
    }

    completed = 0;
    int totalEdges = (int)globalTopologyEdges.size();
    for (size_t i = 0; i < globalTopologyEdges.size(); i++) {
        auto& te = globalTopologyEdges[i]; const auto& pts = te.originalPoints;
        if (pts.size() < 2) { te.simplifiedPoints = pts; completed++; continue; }

        int finalFactor = simplifyAmount;
        if (te.isUnregisteredBoundary || te.isMapOuterEdge) {
            finalFactor = 1;
        } else if (te.isCoastline) {
            if (finalFactor > 2) finalFactor = 2;
        }

        std::vector<MapPoint> simPts;
        simPts.push_back(pts.front());
        for (size_t k = 1; k < pts.size() - 1; k++) {
            if (k % finalFactor == 0) simPts.push_back(pts[k]);
        }
        if (simPts.back() != pts.back()) simPts.push_back(pts.back());
        te.simplifiedPoints = simPts;

        completed++;
        if (completed % 200 == 0 || completed == totalEdges) {
            updateSmoothProgress(loc.step3, 3, completed.load(), totalEdges);
        }
    }
    updateSmoothProgress(loc.step3, 3, totalEdges, totalEdges);

    completed = 0;
    int totalIslands = (int)allIslands.size();
    for (size_t i = 0; i < allIslands.size(); i++) {
        auto& isl = allIslands[i];
        auto edgeListIds = islandToEdgesMap[isl.id];
        if (edgeListIds.empty()) { completed++; continue; }

        struct ConnectableEdge { size_t id; MapPoint pStart; MapPoint pEnd; };
        std::vector<ConnectableEdge> cEdges;
        for (size_t eId : edgeListIds) {
            const auto& te = globalTopologyEdges[eId];
            cEdges.push_back({ te.edgeId, te.simplifiedPoints.front(), te.simplifiedPoints.back() });
        }

        std::vector<bool> edgeUsed(cEdges.size(), false);
        std::vector<std::vector<MapPoint>> gatheredLoops;

        for (size_t startIdx = 0; startIdx < cEdges.size(); startIdx++) {
            if (edgeUsed[startIdx]) continue;

            std::vector<MapPoint> currentLoop;
            edgeUsed[startIdx] = true;

            const auto& firstEdgePoints = globalTopologyEdges[cEdges[startIdx].id].simplifiedPoints;
            for (const auto& p : firstEdgePoints) currentLoop.push_back(p);
            MapPoint currentTail = firstEdgePoints.back();

            size_t safetyCounter = 0;
            while (safetyCounter++ < cEdges.size() * 2) {
                size_t nextFound = -1;
                bool nextForward = true;

                for (size_t k = 0; k < cEdges.size(); k++) {
                    if (edgeUsed[k]) continue;
                    if (cEdges[k].pStart == currentTail) {
                        nextFound = k; nextForward = true; break;
                    }
                    if (cEdges[k].pEnd == currentTail) {
                        nextFound = k; nextForward = false; break;
                    }
                }

                if (nextFound == -1) break;

                edgeUsed[nextFound] = true;
                const auto& tePoints = globalTopologyEdges[cEdges[nextFound].id].simplifiedPoints;

                if (nextForward) {
                    for (size_t idx = 1; idx < tePoints.size(); idx++) currentLoop.push_back(tePoints[idx]);
                    currentTail = tePoints.back();
                } else {
                    for (int idx = (int)tePoints.size() - 2; idx >= 0; idx--) currentLoop.push_back(tePoints[idx]);
                    currentTail = tePoints.front();
                }
            }

            if (currentLoop.size() >= 3) {
                if (currentLoop.front() != currentLoop.back()) currentLoop.push_back(currentLoop.front());
                gatheredLoops.push_back(currentLoop);
            }
        }

        isl.finalLoops = gatheredLoops;
        completed++;
        if (completed % 100 == 0 || completed == totalIslands) {
            updateSmoothProgress(loc.step4, 4, completed.load(), totalIslands);
        }
    }
    updateSmoothProgress(loc.step4, 4, totalIslands, totalIslands);

    std::unordered_map<int, std::vector<std::vector<MapPoint>>> colorToFinalLoops;
    for (const auto& isl : allIslands) {
        for (const auto& loop : isl.finalLoops) {
            if (loop.size() >= 3) colorToFinalLoops[isl.colorKey].push_back(loop);
        }
    }

    auto formatColorPoints = [&](int colorKey) -> std::pair<std::string, std::string> {
        if (colorToFinalLoops.find(colorKey) == colorToFinalLoops.end()) return {"", ""};
        const auto& islandLoops = colorToFinalLoops[colorKey];
        std::vector<MapPoint> masterBridgePoints;

        for (size_t k = 0; k < islandLoops.size(); k++) {
            std::vector<MapPoint> cleanGameLoop;
            for (const auto& pt : islandLoops[k]) {
                cleanGameLoop.push_back({ pt.x / 2, pt.y / 2 });
            }
            cleanGameLoop.erase(std::unique(cleanGameLoop.begin(), cleanGameLoop.end()), cleanGameLoop.end());
            if (cleanGameLoop.size() < 3) continue;
            if (cleanGameLoop.front() != cleanGameLoop.back()) cleanGameLoop.push_back(cleanGameLoop.front());

            if (masterBridgePoints.empty()) {
                masterBridgePoints = cleanGameLoop;
            } else {
                MapPoint bridgeReturnPoint = masterBridgePoints.back();
                masterBridgePoints.push_back(cleanGameLoop.front());
                for (size_t idx = 1; idx < cleanGameLoop.size(); idx++) masterBridgePoints.push_back(cleanGameLoop[idx]);
                masterBridgePoints.push_back(bridgeReturnPoint);
            }
        }

        if (masterBridgePoints.size() < 4) return {"", ""};

        std::stringstream xs, ys;
        for (size_t i = 0; i < masterBridgePoints.size(); i++) {
            xs << masterBridgePoints[i].x; ys << masterBridgePoints[i].y;
            if (i + 1 < masterBridgePoints.size()) { xs << ","; ys << ","; }
        }
        return {xs.str(), ys.str()};
    };

    std::ofstream out("MAP_POINTS.txt");
    bool firstLineWritten = false;
    int targetProvId = 1;

    auto writeRowPair = [&](const std::string& xs, const std::string& ys) {
        if (firstLineWritten) out << "\n";
        out << (xs.empty() ? "0" : xs) << "\n" << (ys.empty() ? "0" : ys);
        firstLineWritten = true;
    };

    while (targetProvId <= maxRegisteredProvId) {
        if (provIdToColorKey.find(targetProvId) != provIdToColorKey.end()) {
            int cKey = provIdToColorKey[targetProvId];
            auto rows = formatColorPoints(cKey);
            writeRowPair(rows.first, rows.second);
        }
        else {
            if (!unregisteredColors.empty()) {
                int fallbackColorKey = unregisteredColors.front();
                unregisteredColors.erase(unregisteredColors.begin());

                auto rows = formatColorPoints(fallbackColorKey);
                writeRowPair(rows.first, rows.second);
            } else {
                writeRowPair("0", "0");
            }
        }
        targetProvId++;
    }

    while (!unregisteredColors.empty()) {
        int fallbackColorKey = unregisteredColors.front();
        unregisteredColors.erase(unregisteredColors.begin());

        auto rows = formatColorPoints(fallbackColorKey);
        writeRowPair(rows.first, rows.second);
        targetProvId++;
    }

    SendMessage(g_progressBar, PBM_SETPOS, 10000, 0);

    auto end = std::chrono::high_resolution_clock::now();
    double s = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0;
    wchar_t buf[256];
    swprintf(buf, 256, loc.done, s, missingCount);
    setText(g_timeText, buf);

    // 轉換結束後，啟用關閉按鈕
    EnableWindow(g_closeButton, TRUE);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_CREATE: {
            INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_PROGRESS_CLASS }; InitCommonControlsEx(&icc);
            int dpi = GetWindowDpi(hwnd); g_font = createUIFontForDpi(dpi);

            g_progressBar = CreateWindowExW(0, PROGRESS_CLASSW, NULL, WS_CHILD | WS_VISIBLE, ScalePixels(20, dpi), ScalePixels(20, dpi), ScalePixels(620, dpi), ScalePixels(28, dpi), hwnd, NULL, NULL, NULL);
            g_statusText = CreateWindowExW(0, L"STATIC", loc.ready, WS_CHILD | WS_VISIBLE, ScalePixels(20, dpi), ScalePixels(60, dpi), ScalePixels(620, dpi), ScalePixels(25, dpi), hwnd, NULL, NULL, NULL);
            g_timeText = CreateWindowExW(0, L"STATIC", loc.tip_files, WS_CHILD | WS_VISIBLE, ScalePixels(20, dpi), ScalePixels(90, dpi), ScalePixels(620, dpi), ScalePixels(25, dpi), hwnd, NULL, NULL, NULL);

            HWND lbl = CreateWindowExW(0, L"STATIC", loc.simplify, WS_CHILD | WS_VISIBLE, ScalePixels(20, dpi), ScalePixels(120, dpi), ScalePixels(80, dpi), ScalePixels(25, dpi), hwnd, NULL, NULL, NULL);
            g_editBox = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"1", WS_CHILD | WS_VISIBLE | ES_NUMBER, ScalePixels(100, dpi), ScalePixels(120, dpi), ScalePixels(80, dpi), ScalePixels(26, dpi), hwnd, (HMENU)1001, NULL, NULL);
            g_button = CreateWindowExW(0, L"BUTTON", loc.start, WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, ScalePixels(195, dpi), ScalePixels(118, dpi), ScalePixels(140, dpi), ScalePixels(32, dpi), hwnd, (HMENU)1002, NULL, NULL);
            g_closeButton = CreateWindowExW(0, L"BUTTON", loc.close, WS_CHILD | WS_VISIBLE, ScalePixels(345, dpi), ScalePixels(118, dpi), ScalePixels(90, dpi), ScalePixels(32, dpi), hwnd, (HMENU)1003, NULL, NULL);
            g_helpButton = CreateWindowExW(0, L"BUTTON", L"?", WS_CHILD | WS_VISIBLE, ScalePixels(445, dpi), ScalePixels(118, dpi), ScalePixels(35, dpi), ScalePixels(32, dpi), hwnd, (HMENU)1004, NULL, NULL);

            applyFont(g_progressBar); applyFont(g_statusText); applyFont(g_timeText); applyFont(lbl); applyFont(g_editBox); applyFont(g_button); applyFont(g_closeButton); applyFont(g_helpButton);
            EnableWindow(g_closeButton, FALSE); break;
        }
        case 0x02E0: { // WM_DPICHANGED
            int nDpi = HIWORD(wp);
            RECT* prc = (RECT*)lp;
            if (g_font) DeleteObject(g_font);
            g_font = createUIFontForDpi(nDpi);

            SetWindowPos(hwnd, NULL, prc->left, prc->top, prc->right - prc->left, prc->bottom - prc->top, SWP_NOZORDER | SWP_NOACTIVATE);

            SetWindowPos(g_progressBar, NULL, ScalePixels(20, nDpi), ScalePixels(20, nDpi), ScalePixels(620, nDpi), ScalePixels(28, nDpi), SWP_NOZORDER | SWP_NOACTIVATE);
            SetWindowPos(g_statusText, NULL, ScalePixels(20, nDpi), ScalePixels(60, nDpi), ScalePixels(620, nDpi), ScalePixels(25, nDpi), SWP_NOZORDER | SWP_NOACTIVATE);
            SetWindowPos(g_timeText, NULL, ScalePixels(20, nDpi), ScalePixels(90, nDpi), ScalePixels(620, nDpi), ScalePixels(25, nDpi), SWP_NOZORDER | SWP_NOACTIVATE);

            SetWindowPos(g_editBox, NULL, ScalePixels(100, nDpi), ScalePixels(120, nDpi), ScalePixels(80, nDpi), ScalePixels(26, nDpi), SWP_NOZORDER | SWP_NOACTIVATE);
            SetWindowPos(g_button, NULL, ScalePixels(195, nDpi), ScalePixels(118, nDpi), ScalePixels(140, nDpi), ScalePixels(32, nDpi), SWP_NOZORDER | SWP_NOACTIVATE);
            SetWindowPos(g_closeButton, NULL, ScalePixels(345, nDpi), ScalePixels(118, nDpi), ScalePixels(90, nDpi), ScalePixels(32, nDpi), SWP_NOZORDER | SWP_NOACTIVATE);
            SetWindowPos(g_helpButton, NULL, ScalePixels(445, nDpi), ScalePixels(118, nDpi), ScalePixels(35, nDpi), ScalePixels(32, nDpi), SWP_NOZORDER | SWP_NOACTIVATE);

            applyFont(g_progressBar); applyFont(g_statusText); applyFont(g_timeText); applyFont(g_editBox); applyFont(g_button); applyFont(g_closeButton); applyFont(g_helpButton);
            InvalidateRect(hwnd, NULL, TRUE); break;
        }
        case WM_COMMAND: {
            if (LOWORD(wp) == 1002) {
                char buf[64]; GetWindowTextA(g_editBox, buf, 64); simplifyAmount = atoi(buf); if (simplifyAmount < 1) simplifyAmount = 1;
                EnableWindow(g_button, FALSE); std::thread(runGenerator).detach();
            } else if (LOWORD(wp) == 1003) {
                PostQuitMessage(0);
            } else if (LOWORD(wp) == 1004) {
                MessageBoxW(hwnd, loc.help_content, loc.help_title, MB_OK | MB_ICONINFORMATION);
            }
            break;
        }
        case WM_DESTROY: { DeleteObject(g_font); PostQuitMessage(0); break; }
        default: return DefWindowProcW(hwnd, msg, wp, lp);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    InitLanguage();
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        typedef BOOL(WINAPI* SPA)(DPI_AWARENESS_CONTEXT); auto pSPA = (SPA)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (pSPA) pSPA(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    }

    HICON hAppIcon = LoadIconW(hInst, MAKEINTRESOURCEW(1));

    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"HOI4MapShaperWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = hAppIcon;

    if (!RegisterClassW(&wc)) return 0;
    HDC scr = GetDC(NULL); int dDpi = GetDeviceCaps(scr, LOGPIXELSX); ReleaseDC(NULL, scr);

    g_hwndMain = CreateWindowExW(0, L"HOI4MapShaperWindow", loc.window_title, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, MulDiv(660, dDpi, 96), MulDiv(225, dDpi, 96), nullptr, nullptr, hInst, NULL);
    ShowWindow(g_hwndMain, nCmdShow); MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessage(&msg); }
    return 0;
}
