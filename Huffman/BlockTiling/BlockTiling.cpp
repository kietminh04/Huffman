#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <omp.h>     
#include <fstream>   
#include <queue>     

using namespace std;

const int BLOCK_SIZE = 10 * 1024 * 1024;

struct Node {
    char ch;
    int freq;
    Node* left, * right;
    Node(char c, int f, Node* l = nullptr, Node* r = nullptr) { ch = c; freq = f; left = l; right = r; }
};

struct Compare {
    bool operator()(Node* l, Node* r) { return l->freq > r->freq; }
};

void generateCodes(Node* root, string str, map<char, string>& huffmanCode) {
    if (!root) return;
    if (!root->left && !root->right) huffmanCode[root->ch] = str;
    generateCodes(root->left, str + "0", huffmanCode);
    generateCodes(root->right, str + "1", huffmanCode);
}

void readBlockFromDisk(char* buffer, int size, int blockIndex) {
    string pattern = "tinh toan song song de tai 14 ma hoa huffman";
    int patternLen = pattern.length();

#pragma omp parallel for
    for (int i = 0; i < size; i++) {
        buffer[i] = pattern[(blockIndex * size + i) % patternLen];
    }
}

// 1. ĐẾM TẦN SUẤT (BLOCKING)
void countFrequencyBlocked(long long totalSize, long long freq[256]) {
    for (int i = 0; i < 256; i++) freq[i] = 0;
    int numBlocks = (totalSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    char* buffer = new char[BLOCK_SIZE];

    cout << "--- [PHASE 1] DEM TAN SUAT (BLOCKING TECHNIQUE) ---\n";

    for (int b = 0; b < numBlocks; b++) {
        int currentBlockSize = (b == numBlocks - 1) ? (totalSize % BLOCK_SIZE) : BLOCK_SIZE;
        if (currentBlockSize == 0) currentBlockSize = BLOCK_SIZE;

        readBlockFromDisk(buffer, currentBlockSize, b);

#pragma omp parallel
        {
            long long local_freq[256] = { 0 };
#pragma omp for nowait
            for (int i = 0; i < currentBlockSize; i++) {
                local_freq[(unsigned char)buffer[i]]++;
            }
#pragma omp critical
            {
                for (int i = 0; i < 256; i++) freq[i] += local_freq[i];
            }
        }
    }
    cout << "Hoan thanh dem tan suat!\n";
    delete[] buffer;
}

// 2. MÃ HÓA (BLOCKING)
void encodeBlocked(long long totalSize, map<char, string>& codeMap) {
    int numBlocks = (totalSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
    char* inputBuffer = new char[BLOCK_SIZE];

    cout << "\n--- [PHASE 2] MA HOA DU LIEU (BLOCKING TECHNIQUE) ---\n";

    long long totalBits = 0;

    for (int b = 0; b < numBlocks; b++) {
        int currentBlockSize = (b == numBlocks - 1) ? (totalSize % BLOCK_SIZE) : BLOCK_SIZE;
        if (currentBlockSize == 0) currentBlockSize = BLOCK_SIZE;

        readBlockFromDisk(inputBuffer, currentBlockSize, b);

        long long blockBits = 0;
#pragma omp parallel reduction(+:blockBits)
        {
            long long localBits = 0;
#pragma omp for
            for (int i = 0; i < currentBlockSize; i++) {
                localBits += codeMap[inputBuffer[i]].length();
            }
            blockBits += localBits;
        }
        totalBits += blockBits;
    }
    cout << "Kich thuoc sau nen:  " << totalBits / 8 / 1024 / 1024 << " MB\n";

    delete[] inputBuffer;
}

int main() {
    // GIẢ LẬP FILE 1 GB
    long long FILE_SIZE = 10LL * 1024 * 1024;

    cout << "GIA LAP FILE KICH THUOC: " << FILE_SIZE / 1024 / 1024 << " MB\n";
    cout << "KICH THUOC MOI BLOCK:  " << BLOCK_SIZE / 1024 / 1024 << " MB\n";
    cout << "SO LUONG THREADS:      " << omp_get_max_threads() << "\n\n";

    double start = omp_get_wtime();

    // 1. Đếm tần suất
    long long freq[256];
    countFrequencyBlocked(FILE_SIZE, freq);

    // 2. Dựng cây (Tuần tự)
    priority_queue<Node*, vector<Node*>, Compare> pq;
    for (int i = 0; i < 256; i++) if (freq[i] > 0) pq.push(new Node((char)i, freq[i]));

    while (pq.size() != 1) {
        Node* l = pq.top(); pq.pop();
        Node* r = pq.top(); pq.pop();
        pq.push(new Node('\0', l->freq + r->freq, l, r));
    }
    Node* root = pq.top();

    map<char, string> huffmanCode;
    generateCodes(root, "", huffmanCode);

    // 3. Mã hóa (Block)
    encodeBlocked(FILE_SIZE, huffmanCode);

    double end = omp_get_wtime();
    cout << "\n------------------------------------------------\n";
    cout << "TONG THOI GIAN XU LY: " << end - start << " giay\n";
    // Tính tốc độ MB/s
    cout << "TOC DO XU LY: " << (FILE_SIZE / 1024 / 1024) / (end - start) << " MB/s\n";

    return 0;
}