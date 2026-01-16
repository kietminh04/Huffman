#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <omp.h>
#include <iomanip>
#include <queue>

using namespace std;

// --- CẤU HÌNH ---
long long FILE_SIZE = 10LL * 1024 * 1024;
const int BLOCK_SIZE = 1 * 1024 * 1024;

// --- CẤU TRÚC CHO CANONICAL ---
struct Node {
    char id;
    long long freq;
    Node* left, * right;

    Node(char c, long long f, Node* l = nullptr, Node* r = nullptr)
        : id(c), freq(f), left(l), right(r) {
    }
};

struct CompareNode {
    bool operator()(Node* l, Node* r) {
        return l->freq > r->freq;
    }
};

struct CanonicalInfo {
    char id;
    int cl;
    string codeword;
};

void readBlockFromDisk(char* buffer, int size, int blockIndex) {
    string pattern = "tinh toan song song de tai 14 ma hoa huffman";
    int pLen = pattern.length();
    long long startOffset = (long long)blockIndex * BLOCK_SIZE;

#pragma omp parallel for
    for (int i = 0; i < size; i++) {
        buffer[i] = pattern[(startOffset + i) % pLen];
    }
}

// =============================================================
// PHA 1: ĐẾM TẦN SUẤT 
// =============================================================
void countFrequencyBlocked(long long totalSize, long long freq[256]) {
    for (int i = 0; i < 256; i++) freq[i] = 0;

    int numBlocks = (totalSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    char* buffer = new char[BLOCK_SIZE];

    cout << "--- [PHASE 1] QUET TAN SUAT (BLOCKING) ---\n";

    for (int b = 0; b < numBlocks; b++) {
        int currentSize = (b == numBlocks - 1) ? (totalSize % BLOCK_SIZE) : BLOCK_SIZE;
        readBlockFromDisk(buffer, currentSize, b);

#pragma omp parallel
        {
            long long local_freq[256] = { 0 };
#pragma omp for nowait
            for (int i = 0; i < currentSize; i++) {
                local_freq[(unsigned char)buffer[i]]++;
            }
#pragma omp critical
            {
                for (int i = 0; i < 256; i++) freq[i] += local_freq[i];
            }
        }
    }
    delete[] buffer;
    cout << "-> Xong Phase 1!\n";
}

// Hàm đệ quy đo chiều cao cây 
void getCodeLengths(Node* root, int depth, vector<CanonicalInfo>& listInfo) {
    if (!root) return;
    if (!root->left && !root->right) {
        listInfo.push_back({ root->id, depth, "" });
    }
    getCodeLengths(root->left, depth + 1, listInfo);
    getCodeLengths(root->right, depth + 1, listInfo);
}

// =============================================================
// PHA 2: CANONICAL 
// =============================================================

void generateCanonicalCodesFast(long long freq[256], int lengthTable[256], map<char, string>& debugMap) {
    cout << "\n--- [PHASE 2] TAO MA CANONICAL (FAST PQ) ---\n";

    priority_queue<Node*, vector<Node*>, CompareNode> pq;
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) pq.push(new Node((char)i, freq[i]));
    }

    if (pq.empty()) return;

    while (pq.size() > 1) {
        Node* l = pq.top(); pq.pop();
        Node* r = pq.top(); pq.pop();
        pq.push(new Node('\0', l->freq + r->freq, l, r));
    }
    Node* root = pq.top();

    vector<CanonicalInfo> listInfo;
    getCodeLengths(root, 0, listInfo);

    sort(listInfo.begin(), listInfo.end(), [](const CanonicalInfo& a, const CanonicalInfo& b) {
        if (a.cl != b.cl) return a.cl < b.cl;
        return a.id < b.id;
        });

    long long currentCode = 0;
    cout << "-> Bang ma Canonical (Demo 3 ky tu dau):\n";

    for (int i = 0; i < listInfo.size(); i++) {
        if (i > 0) {
            currentCode++;
            int lenDiff = listInfo[i].cl - listInfo[i - 1].cl;
            if (lenDiff > 0) currentCode = currentCode << lenDiff;
        }
        else {
            currentCode = 0;
        }

        lengthTable[(unsigned char)listInfo[i].id] = listInfo[i].cl;

        string s = "";
        long long val = currentCode;
        for (int k = 0; k < listInfo[i].cl; k++) {
            s = ((val % 2 == 0) ? "0" : "1") + s;
            val /= 2;
        }
        debugMap[listInfo[i].id] = s;

        if (i < 3) cout << "   Char '" << listInfo[i].id << "' | Len: " << listInfo[i].cl << " | Code: " << s << endl;
    }
}

// =============================================================
// PHA 3: TÍNH KÍCH THƯỚC 
// =============================================================
void calculateSizeFast(long long totalSize, int lengthTable[256]) {
    cout << "\n--- [PHASE 3] TINH KICH THUOC (FAST ARRAY LOOKUP) ---\n";

    int numBlocks = (totalSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    char* buffer = new char[BLOCK_SIZE];
    long long totalBits = 0;

    for (int b = 0; b < numBlocks; b++) {
        int currentSize = (b == numBlocks - 1) ? (totalSize % BLOCK_SIZE) : BLOCK_SIZE;
        readBlockFromDisk(buffer, currentSize, b);

        long long blockBits = 0;

#pragma omp parallel reduction(+:blockBits)
        {
            long long localBits = 0;
#pragma omp for
            for (int i = 0; i < currentSize; i++) {
                // TỐI ƯU HÓA: Truy cập mảng O(1) thay vì Map O(logN)
                localBits += lengthTable[(unsigned char)buffer[i]];
            }
            blockBits += localBits;
        }
        totalBits += blockBits;
    }
    delete[] buffer;

    double originalMB = (double)totalSize / 1024 / 1024;
    double compressedMB = (double)totalBits / 8 / 1024 / 1024;

    cout << fixed << setprecision(2);
    cout << "\n=== KET QUA SO SANH ===\n";
    cout << "Kich thuoc GOC:       " << originalMB << " MB\n";
    cout << "Kich thuoc SAU NEN:   " << compressedMB << " MB\n";
    cout << "Ti le nen:            " << (compressedMB / originalMB) * 100 << "%\n";
}

// =============================================================
// PHA 4: GIẢI MÃ (DECODE) 
// =============================================================
struct DecodeNode {
    char id;
    DecodeNode* left = nullptr, * right = nullptr;
};

// Hàm dựng cây giải mã từ bảng mã Canonical
void buildDecodeTree(DecodeNode* root, map<char, string>& codeMap) {
    // --- ĐÃ SỬA ĐOẠN NÀY ĐỂ HẾT LỖI ĐỎ ---
    for (auto const& pair : codeMap) {
        char key = pair.first;
        string val = pair.second;

        DecodeNode* curr = root;
        for (char bit : val) {
            if (bit == '0') {
                if (!curr->left) curr->left = new DecodeNode();
                curr = curr->left;
            }
            else {
                if (!curr->right) curr->right = new DecodeNode();
                curr = curr->right;
            }
        }
        curr->id = key; // Gán ký tự vào lá
    }
}

void decodeBlocked(long long totalSize, map<char, string>& codeMap) {
    cout << "\n--- [PHASE 4] GIAI MA (DECODE TEST) ---\n";

    // 1. Dựng lại cây giải mã từ bảng mã
    DecodeNode* root = new DecodeNode();
    buildDecodeTree(root, codeMap);

    // 2. Giả lập đọc file nén và giải mã
    char* buffer = new char[BLOCK_SIZE];
    readBlockFromDisk(buffer, BLOCK_SIZE, 0);

    string encodedBits = "";
    for (int i = 0; i < 100; i++) encodedBits += codeMap[buffer[i]];

    cout << "Input Bits: " << encodedBits.substr(0, 50) << "...\n";
    cout << "Output:     ";

    // Bắt đầu giải mã (Decode)
    DecodeNode* curr = root;
    for (char bit : encodedBits) {
        if (bit == '0') curr = curr->left;
        else curr = curr->right;

        if (!curr->left && !curr->right) { // Đến lá
            cout << curr->id;
            curr = root; // Quay về gốc
        }
    }
    cout << "\n-> Giai ma thanh cong!\n";
    delete[] buffer;
}


int main() {
    omp_set_num_threads(1);
    cout << "CHUONG TRINH HUFFMAN CANONICAL (BLOCK + ARRAY OPTIMIZED)\n";
    cout << "File Size: " << FILE_SIZE / 1024 / 1024 << " MB\n";
    cout << "Threads:   " << omp_get_max_threads() << "\n\n";

    double start = omp_get_wtime();

    // 1. Đếm tần suất
    long long freq[256];
    countFrequencyBlocked(FILE_SIZE, freq);

    // 2. Tạo mã Canonical 
    int lengthTable[256] = { 0 };
    map<char, string> debugMap;
    generateCanonicalCodesFast(freq, lengthTable, debugMap);

    // 3. Tính kích thước 
    calculateSizeFast(FILE_SIZE, lengthTable);

    // 4. Giải mã (Đã thêm)
    decodeBlocked(FILE_SIZE, debugMap);

    double end = omp_get_wtime();
    cout << "\nTong thoi gian chay: " << end - start << " giay\n";
    cout << "Toc do xu ly:         " << (FILE_SIZE / 1024 / 1024) / (end - start) << " MB/s\n";

    return 0;
}