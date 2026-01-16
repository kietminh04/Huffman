#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <string>
#include <map>
#include <omp.h> 

using namespace std;

long long FILE_SIZE = 10LL * 1024 * 1024;

struct LeafNode {
    char id;
    int index;
    long long freq;
    int parent;
    int cl;
    string codeword;
};

struct InternalNode {
    int id;
    long long freq;
    int parent;
};

string toBinary(long long val, int len) {
    if (len == 0) return "";
    string s = "";
    for (int i = 0; i < len; i++) {
        s = ((val % 2 == 0) ? "0" : "1") + s;
        val /= 2;
    }
    return s;
}

bool compareLeaves(const LeafNode& a, const LeafNode& b) {
    if (a.cl != b.cl) return a.cl < b.cl;
    return a.id < b.id;
}

// --- LOGIC TẠO MÃ CANONICAL + BẢNG ĐỘ DÀI (QUAN TRỌNG) ---
// Thêm tham số lengthTable[] để lưu độ dài vào mảng
void generateCanonicalCodes(long long freq[256], map<char, string>& codeMap, int lengthTable[256]) {
    vector<LeafNode> leaves;
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) leaves.push_back({ (char)i, (int)leaves.size(), freq[i], -1, 0, "" });
    }

    int n = leaves.size();
    if (n == 0) return;

    // ... (Phần dựng cây logic giữ nguyên) ...
    vector<InternalNode> iNodes;
    vector<int> activeRoots;
    for (int i = 0; i < n; i++) activeRoots.push_back(i);

    int iNodeCount = 0;
    while (activeRoots.size() > 1) {
        int idx1 = -1, idx2 = -1;
        long long minFreq1 = -1, minFreq2 = -1;
        int removePos1 = -1, removePos2 = -1;

        auto getFreq = [&](int rootId) {
            return (rootId >= 0) ? leaves[rootId].freq : iNodes[-(rootId + 1)].freq;
        };

        for (int i = 0; i < activeRoots.size(); i++) {
            long long f = getFreq(activeRoots[i]);
            if (minFreq1 == -1 || f < minFreq1) { minFreq1 = f; idx1 = activeRoots[i]; removePos1 = i; }
        }
        activeRoots.erase(activeRoots.begin() + removePos1);

        for (int i = 0; i < activeRoots.size(); i++) {
            long long f = getFreq(activeRoots[i]);
            if (minFreq2 == -1 || f < minFreq2) { minFreq2 = f; idx2 = activeRoots[i]; removePos2 = i; }
        }
        activeRoots.erase(activeRoots.begin() + removePos2);

        iNodes.push_back({ iNodeCount, minFreq1 + minFreq2, -1 });
        if (idx1 >= 0) leaves[idx1].parent = iNodeCount; else iNodes[-(idx1 + 1)].parent = iNodeCount;
        if (idx2 >= 0) leaves[idx2].parent = iNodeCount; else iNodes[-(idx2 + 1)].parent = iNodeCount;

        for (int i = 0; i < n; i++) {
            int curr = i;
            bool isDescendant = false;
            if (curr == idx1 || curr == idx2) isDescendant = true;
            else if (leaves[i].parent != -1) {
                int p = leaves[i].parent;
                int target1 = (idx1 < 0) ? -(idx1 + 1) : -999;
                int target2 = (idx2 < 0) ? -(idx2 + 1) : -999;
                while (p != -1) {
                    if (p == target1 || p == target2) { isDescendant = true; break; }
                    p = iNodes[p].parent;
                }
            }
            if (isDescendant) leaves[i].cl++;
        }
        activeRoots.push_back(-(iNodeCount + 1));
        iNodeCount++;
    }
    // ... (Hết phần dựng cây logic) ...

    sort(leaves.begin(), leaves.end(), compareLeaves);
    long long currentCode = 0;
    int currentLen = leaves[0].cl;

    for (int i = 0; i < n; i++) {
        if (i > 0) {
            currentCode++;
            int lenDiff = leaves[i].cl - leaves[i - 1].cl;
            if (lenDiff > 0) { currentCode = currentCode << lenDiff; currentLen = leaves[i].cl; }
        }
        else { currentCode = 0; currentLen = leaves[i].cl; }

        leaves[i].codeword = toBinary(currentCode, currentLen);
        codeMap[leaves[i].id] = leaves[i].codeword;
        
        // --- ĐÂY LÀ PHẦN QUAN TRỌNG: Lưu độ dài vào mảng ---
        lengthTable[(unsigned char)leaves[i].id] = leaves[i].cl;
    }
}

int main() {
    cout << "=== CANONICAL HUFFMAN (OPTIMIZED WITH ARRAY LOOKUP) ===\n";
    cout << "File Size: " << FILE_SIZE / 1024 / 1024 << " MB\n";
    cout << "Threads:   " << omp_get_max_threads() << "\n\n";

    // 1. SINH DỮ LIỆU 
    cout << "Dang sinh du lieu vao RAM... ";
    string text;
    try {
        text.resize(FILE_SIZE); 
    }
    catch (std::bad_alloc& e) {
        cout << "\nLOI: Tran bo nho RAM! Hay giam FILE_SIZE xuong.\n";
        return -1;
    }

    string pattern = " tinh toan song song de tai 14 ma hoa huffman";
    int patLen = pattern.length();

#pragma omp parallel for
    for (long long i = 0; i < FILE_SIZE; i++) {
        text[i] = pattern[i % patLen];
    }
    cout << "Xong!\n";

    double start = omp_get_wtime();

    // 2. ĐẾM TẦN SUẤT 
    long long freq[256] = { 0 };
#pragma omp parallel
    {
        long long local_freq[256] = { 0 };
#pragma omp for
        for (long long i = 0; i < FILE_SIZE; i++) {
            local_freq[(unsigned char)text[i]]++;
        }
#pragma omp critical
        {
            for (int i = 0; i < 256; i++) freq[i] += local_freq[i];
        }
    }

    // 3. TẠO MÃ CANONICAL + BẢNG ĐỘ DÀI MẢNG
    map<char, string> codeMap;
    int lengthTable[256] = { 0 }; // <--- Mảng thần thánh
    generateCanonicalCodes(freq, codeMap, lengthTable);

    // 4. TÍNH KÍCH THƯỚC (DÙNG MẢNG - SIÊU TỐC)
    // Thay vì dùng codeMap[text[i]], ta dùng lengthTable[text[i]]
    long long totalBits = 0;
#pragma omp parallel reduction(+:totalBits)
    {
        long long localBits = 0;
#pragma omp for
        for (long long i = 0; i < FILE_SIZE; i++) {
            // --- TỐI ƯU HÓA Ở ĐÂY: Truy cập mảng O(1) ---
            localBits += lengthTable[(unsigned char)text[i]]; 
        }
        totalBits += localBits;
    }

    double end = omp_get_wtime();

    // BÁO CÁO
    double originalMB = (double)FILE_SIZE / 1024 / 1024;
    double compressedMB = (double)totalBits / 8 / 1024 / 1024;

    cout << fixed << setprecision(2);
    cout << "\n=== KET QUA ===\n";
    cout << "File Goc:             " << originalMB << " MB\n";
    cout << "File Nen:             " << compressedMB << " MB\n";
    cout << "Ti le nen:            " << (compressedMB / originalMB) * 100 << "%\n";
    cout << "Thoi gian chay:       " << end - start << " giay\n";
    cout << "Toc do xu ly:         " << (FILE_SIZE / 1024 / 1024) / (end - start) << " MB/s\n";

    return 0;
}