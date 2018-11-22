//
// Created by timur on 19.11.2018.
//

#ifndef UNTITLED21_HUFFMAN_H
#define UNTITLED21_HUFFMAN_H

typedef unsigned char byte;

struct IInputStream {
    // Возвращает false, если поток закончился
    virtual bool Read(byte& value) = 0;
    virtual bool extraRead(byte &value) = 0;
};

struct IOutputStream {
    virtual void Write(byte value) = 0;
};


static void copyStream(IInputStream& input, IOutputStream& output)
{
    byte value;
    while (input.Read(value))
    {
        output.Write(value);
    }
}

void Encode(IInputStream& original, IOutputStream& compressed);

void Decode(IInputStream& compressed, IOutputStream& original);

#endif //UNTITLED21_HUFFMAN_H
