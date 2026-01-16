#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <map>
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


void countFrequencyParallel(const string& text, long long freq[256]) {
    for (int i = 0; i < 256; i++) freq[i] = 0;

#pragma omp parallel
    {
        long long local_freq[256] = { 0 };

#pragma omp for
        for (long long i = 0; i < text.length(); i++) {
            local_freq[(unsigned char)text[i]]++;
        }

#pragma omp critical
        {
            for (int i = 0; i < 256; i++) {
                freq[i] += local_freq[i];
            }
        }
    }
}

string encodeParallel(const string& text, map<char, string>& codeMap) {
    long long n = text.length();
    int max_threads = omp_get_max_threads();
    vector<string> chunks(max_threads);

#pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        int num_threads = omp_get_num_threads();

        long long chunk_size = n / num_threads;
        long long start = thread_id * chunk_size;
        long long end = (thread_id == num_threads -1) ? n : start + chunk_size;

        string local_res = "";
        local_res.reserve((end - start) * 5);

        for (long long i = start; i < end; i++) {
            local_res += codeMap[text[i]];
        }

        chunks[thread_id] = local_res;
    }

    string final_result = "";
    for (int i = 0; i < max_threads; i++) {
        final_result += chunks[i];
    }
    return final_result;
}

int main() {
    cout << "Dang tao du lieu test (Khoang 50-60MB)... Vui long doi...\n";
    string text = "";
    string mau = "huffman parallel processing openmp optimization ";
    for (int i = 0; i < 1500000; i++) {
        text += mau;
    }
    cout << "Da tao xong! Do dai chuoi: " << text.length() << " ky tu.\n\n";

    double start_time = omp_get_wtime();

    long long freq[256];
    countFrequencyParallel(text, freq);

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

    map<char, string> huffmanCode;
    generateCodes(root, "", huffmanCode);

    string encodedString = encodeParallel(text, huffmanCode);

    double end_time = omp_get_wtime();

    cout << "=== KET QUA HUFFMAN SONG SONG (OpenMP) ===\n";
    cout << "So luong CPU tham gia: " << omp_get_max_threads() << " luong (Threads)\n";
    cout << "Tong thoi gian chay:   " << (end_time - start_time) << " giay\n";
    cout << "Kich thuoc goc:        " << text.length() / 1024 / 1024 << " MB\n";
    cout << "Kich thuoc sau nen:    " << encodedString.length() / 8 / 1024 / 1024 << " MB\n";
    cout << "Ti le nen:             " << (float)encodedString.length() / (text.length() * 8) * 100 << "%\n";

    cout << "\nCheck ma hoa 50 bit dau tien:\n" << encodedString.substr(0, 50) << "...\n";

    deleteTree(root);
    return 0;
}