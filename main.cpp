#include <iostream>
#include "Huffman.h"
#include <vector>
#include <cmath>

std::vector<byte> comp;

struct Node {
    Node(byte data_, int freq_) : data(data_), freq(freq_) {};

    Node(Node *left_, Node *right_, int freq_) : left(left_), right(right_), freq(freq_) {};
    int freq; //частота встреч дерева(набора символов) в документе
    byte data;
    Node *left = nullptr; //левое поддерево
    Node *right = nullptr; //правое поддерево
};

void addVector(std::vector<bool> &vector, std::vector<bool> &v) { //копирует один вектор в конец другого
    for (int i = 0; i < v.size(); i++) {
        vector.push_back(v[i]);
    }
}

void
translateToBits(byte b, std::vector<bool> &k) { //перевод последовательности байт в последовательность нулей и единиц
    for (int i = sizeof(b) * 8 - 1; i >= 0; --i) {
        k.push_back((b >> i) & 1);
    }
}

void translateToBytes(std::vector<bool> &k,
                      IOutputStream &compressed) { //перевод последовательности из нулей и единиц в байтовое представление
    byte a = 0;
    int cnt = 0;
    for (int i = 0; i < k.size(); i++) {
        if (cnt < 8) {
            cnt++;
            a += k[i] * (int) pow(2, 7 - i % 8);
        }
        if ((cnt == 8) || (i == k.size() - 1)) {
            cnt = 0;
            compressed.Write(a);
            a = 0;
        }
    }

}

void preOrder(Node *node, std::vector<bool> *codes, std::vector<bool> &k,
              IOutputStream &compressed) { //обход дерева Хаффмана в глубину
    if (!node) {
        return;
    }
    if (node->left == nullptr && node->right == nullptr) {
        codes[node->data] = k;
        compressed.Write(node->data); //Добавляем в сжатый документ символ
        compressed.Write((byte) (k.size())); //Добавляем в сжатый документ зазор шифра, т е количество значимых бит
        translateToBytes(k, compressed); //Добавляем в сжатый документ шифр символа
        return;
    }
    k.push_back(0);
    preOrder(node->left, codes, k, compressed);
    k.pop_back();
    k.push_back(1);
    preOrder(node->right, codes, k, compressed);
    k.pop_back();
}

void minInsertionSort(std::pair<int, Node *> *a, int n) { //сортировка по убыванию (по частоте)
    for (int i = 1; i < n; i++) {
        auto tmp = a[i];
        int j = i - 1;
        for (; j >= 0 && tmp.first > a[j].first; j--) {
            a[j + 1] = a[j];
        }
        a[j + 1] = tmp;
    }
}

void sortLast(std::pair<int, Node *> *a,
              int n) { //вставка добавленного дерева на нужное место в отсортированном по убыванию массиве
    auto tmp = a[n - 1];
    int j = n - 2;
    for (; j >= 0 && tmp.first > a[j].first; j--) {
        a[j + 1] = a[j];
    }
    a[j + 1] = tmp;
}


void Encode(IInputStream &original, IOutputStream &compressed) {
    std::vector<byte> data; //хранит документ по байтам
    byte b = 0;
    int freq[256]; //частота встреч различных символов
    auto *codes = new std::vector<bool>[256]; //хранит шифры для каждого символа (Шифр для символа k хранится в ячейке codes[(int)k])
    for (int i = 0; i < 256; i++) {
        freq[i] = 0;
    }
    while (original.Read(b)) {
        data.push_back(b);
        freq[b]++;
    }

    auto *trees = new std::pair<int, Node *>[256]; //массив, хранящий деревья(изначально символы) для алгоритма Хаффмана (с частотой встречи дерева в документе)
    for (int i = 0; i < 256; i++) {
        Node *node = new Node((byte) i, freq[i]);
        trees[i] = {freq[i], node};
    }
    int numberOfSymbols = 0; //количество различных символов, использующихся в документе
    for (int i = 0; i < 256; i++) {
        if (trees[i].first != 0) {
            numberOfSymbols++;
        }
    }
    compressed.Write(
            (byte) numberOfSymbols); //добавляем в начало сжатого документа количество различных символов, использующихся в документе

    int size = 256;
    minInsertionSort(trees, size); //сортировка символов по частоте встреч в документе по убыванию
    while (trees[size - 1].first == 0) {
        size--;
    }
    while (size > 1) { //построение дерева в алгоритме Хаффмана
        Node *minfreqTree1 = trees[size - 1].second; //берем два дерева с минимальной частотой встреч в документе
        Node *minfreqTree2 = trees[--size - 1].second;
        Node *newTree = new Node(minfreqTree1, minfreqTree2, minfreqTree1->freq + minfreqTree2->freq);
        trees[size - 1] = {newTree->freq, newTree};
        sortLast(trees, size); //вставляем добавленное дерево на нужное место в отсортированном по убыванию массиве
    }
    Node *haffTree = trees[0].second; //дерево шифров в алгоритме Хаффмана

    std::vector<bool> k;
    preOrder(haffTree, codes, k,
             compressed); //Проходимся по дереву, запоминая шифр для каждого символа (+добавляем в начало сжатого документа пары символ + шифр)

    std::vector<bool> compressedVector; //массив, хранящий последовательность 0 и 1, соответствующую зашифрованному документу
    for (int i = 0; i < data.size(); i++) {
        addVector(compressedVector, codes[data[i]]);
    }
    compressed.Write((byte) (compressedVector.size() %
                             8)); //добавляем после пар символ + шифр зазор, т е  количество значимых бит в последнем байте
    translateToBytes(compressedVector, compressed); //переводим compressedVector в байтовое представление
}

void Decode(IInputStream &compressed, IOutputStream &original) {
    byte b = 0;
    //В контесте вместо extraRead используется обычный Read (написанный не мной)
    compressed.extraRead(b);
    int numberOfSymbols = b; //количество различных символов, использующихся в документе
    int cnt = 0; //количество считанных пар символ + шифр
    int index = 0; //хранит числовую интерпретацию символа, то есть индекс в таблице символов и шифров
    std::vector<bool> table[256]; //таблица, хранящая пары символ + шифр(Шифр для символа k хранится в ячейке table[(int)k])
    std::vector<bool> k;
    std::vector<byte> data;

    while (compressed.extraRead(b)) { //считываем зашифрованный документ
        data.push_back(b);
    }

    int iter = 0; //итератор для прохода по массиву data
    int codeLength = 0; //длина шифра для символа
    while (cnt < numberOfSymbols) { //Заполняем таблицу, считывая символы и их шифры
        index = (int) data[iter];
        ++iter;
        codeLength = (int) data[iter];
        ++iter;
        while (codeLength > 0) {
            translateToBits(data[iter], k);
            ++iter;
            addVector(table[index], k);
            k.clear();
            codeLength -= 8;
        }
        for (int i = 0; i < -codeLength; i++) {
            table[index].pop_back(); //удаление ничего не значащих бит из шифра
        }
        cnt++;
    }

    int gap = (8 - (int) data[iter]) % 8; //считываем зазор
    ++iter;

    std::vector<bool> bitseq;
    for (int i = iter; i < data.size(); i++) { //переводим документ в последовательность 0 и 1
        translateToBits(data[i], bitseq);
    }

    for (int i = 0; i < gap; i++) { //удаляем из последовательности 0 и 1 ничего не значащие биты
        bitseq.pop_back();
    }

    std::vector<bool> symbol;
    for (int i = 0; i < bitseq.size(); i++) { //расшифровываем последовательность 0 и 1
        symbol.push_back(bitseq[i]);
        for (int j = 0; j < 256; j++) {
            if (symbol == table[j]) {
                original.Write((byte) j);
                symbol.clear();
                break;
            }
        }
    }
}

class ConsoleInputStream : public IInputStream {
public:
    bool Read(byte &value) override {
        char c = 0;
        std::cin.get(c);
        if (c == '@')
            return false;
        value = (byte) c;
        return true;
    }

    bool extraRead(byte &value) override {
        if (!comp.empty()) {
            value = comp[0];
            for (int i = 0; i < comp.size() - 1; i++) {
                comp[i] = comp[i + 1];
            }
            comp.pop_back();
            return true;
        } else {
            return false;
        }
    }
};

class ConsoleOutputStream : public IOutputStream {
public:
    void Write(byte value) override {
        std::cout.put((char) value);
        comp.push_back(value);
    }
};

int main() {
    ConsoleInputStream inputStream;
    ConsoleOutputStream outputStream;
    Encode(inputStream, outputStream);
    Decode(inputStream, outputStream);
    return 0;
}
