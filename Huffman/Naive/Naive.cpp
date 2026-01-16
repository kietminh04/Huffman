#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <map>
#include <iomanip>
#include <omp.h> 

using namespace std;

struct Node {
    char ch;
    int freq;
    Node* left, * right;

    Node(char character, int frequency, Node* l = nullptr, Node* r = nullptr) {
        ch = character;
        freq = frequency;
        left = l;
        right = r;
    }
};

struct Compare {
    bool operator()(Node* l, Node* r) {
        return l->freq > r->freq;
    }
};

void generateCodes(Node* root, string str, map<char, string>& huffmanCode) {
    if (!root) return;
    // Nếu là lá (Leaf node) thì gán mã
    if (!root->left && !root->right) {
        huffmanCode[root->ch] = str;
    }
    generateCodes(root->left, str + "0", huffmanCode);
    generateCodes(root->right, str + "1", huffmanCode);
}

void deleteTree(Node* root) {
    if (!root) return;
    deleteTree(root->left);
    deleteTree(root->right);
    delete root;
}

int main() {
    long long DATA_SIZE = 10LL * 1024 * 1024;
    string text;
    text.resize(DATA_SIZE);

    cout << "Dang sinh du lieu " << DATA_SIZE / 1024 / 1024 << " MB... ";
    string mau = "tinh toan song song de tai 14 ma hoa huffman";
    int mauLen = mau.length();

#pragma omp parallel for
    for (long long i = 0; i < DATA_SIZE; i++) {
        text[i] = mau[i % mauLen];
    }
    cout << "Xong!\n\n";

    double start = omp_get_wtime();

    // --- BƯỚC 1: ĐẾM TẦN SUẤT  ---

    long long freq[256] = { 0 };
    for (long long i = 0; i < text.length(); i++) {
        freq[(unsigned char)text[i]]++;
    }

    // --- BƯỚC 2: DỰNG CÂY HUFFMAN ---
    priority_queue<Node*, vector<Node*>, Compare> pq;
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            pq.push(new Node((char)i, freq[i]));
        }
    }


    while (pq.size() != 1) {
        Node* left = pq.top(); pq.pop();
        Node* right = pq.top(); pq.pop();
        int sum = left->freq + right->freq;
        pq.push(new Node('\0', sum, left, right));
    }
    Node* root = pq.top();

    // --- BƯỚC 3: TẠO BẢNG MÃ ---
    map<char, string> huffmanCode;
    generateCodes(root, "", huffmanCode);

    // --- BƯỚC 4: TÍNH KÍCH THƯỚC SAU NÉN ---
    long long totalBits = 0;
    for (long long i = 0; i < text.length(); i++) {
        totalBits += huffmanCode[text[i]].length();
    }

    double end = omp_get_wtime();

    // --- KẾT QUẢ ---
    cout << "=== KET QUA NAIVE HUFFMAN ===\n";
    cout << "Thoi gian chay: " << end - start << " giay\n";

    double originalSizeMB = (double)text.length() / 1024 / 1024;
    double compressedSizeMB = (double)totalBits / 8 / 1024 / 1024; // Chia 8 để đổi Bit sang Byte

    cout << fixed << setprecision(2);
    cout << "Kich thuoc TRUOC nen: " << originalSizeMB << " MB\n";
    cout << "Kich thuoc SAU nen:   " << compressedSizeMB << " MB\n";
    cout << "Ti le nen:            " << (compressedSizeMB / originalSizeMB) * 100 << "%\n";

    deleteTree(root);
    return 0;
}