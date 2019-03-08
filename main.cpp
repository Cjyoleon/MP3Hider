#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <cstdio>
#include <bitset>

using namespace std;


void Encode();
void Extract();

string ReadFile(vector<char>& content);
void WriteFile(vector<char>& content, string file_name);
bool findNextFrame(vector<char> &carry, vector<char>::iterator &iter);
unsigned int getBitrate(vector<char>::const_iterator iter);
void writeBitrate(unsigned int bitrate, vector<char>::iterator iter);
unsigned long long GetMP3Capacity(vector<char> &carry);


int main()
{
    int i;
    cout << "Encode: 1\tExtract: 2\n";
    cout << "What to do? ==> ";
    while (cin >> i) {
        if (i == 1) {
            Encode();
        }
        else if (i == 2) {
            Extract();
        }
        cout << "\nWhat to do? ==> ";
    }
}

void Encode() {
    vector<char> carry;
    vector<char> secret;

    cout << "Carrier file: ";
    if (ReadFile(carry) == "")
        return;
    unsigned long long capacity = GetMP3Capacity(carry);

    //Try syncing frames.
    int header = ((carry[6] & 0x7F) << 21) + ((carry[7] & 0x7F) << 14) + ((carry[8] & 0x7F) << 7) + (carry[9] & 0x7F) + 10;
    vector<char>::iterator iter = carry.begin() + header;

    cout << "Secret file (Must be below " << capacity / 8 << " bytes): ";
    string scr_ext,
        scr_path = ReadFile(secret);
    if (scr_path == "")
        return;
    if (secret.size() * 8 > capacity) {
        cout << "The hidden file is too large!";
        return;
    }


    // Get secret file extension.
    for (size_t i = scr_path.size() - 1; i > 0; --i) {
        if (scr_path[i] != '.')
            scr_ext = scr_path[i] + scr_ext;
        else
            break;
    }

    size_t pos = 0;
    // Skip the first frame.
    findNextFrame(carry, iter);

    vector<unsigned int> brs;

    // Write file extension.
    scr_ext += "&#!%";
    while (iter != carry.cend()) {
        unsigned long hd = static_cast<unsigned long>(scr_ext[pos]);
        bitset<8> header{ hd };
        for (size_t i = 0; i < 8; ++i) {
            if (findNextFrame(carry, ++iter)) {
                unsigned int bitrate = getBitrate(iter);
                brs.push_back(bitrate);
                if (bitrate < 13) {
                    cout  << getBitrate(iter) << " ";
                    if ((bitrate % 2) != header[i])
                        writeBitrate(++bitrate, iter);
                    cout  << getBitrate(iter) << endl;
                    continue;
                }
                else if (bitrate == 13)
                    writeBitrate(++bitrate, iter);
                --i;

            }
        }
        ++pos;
        if (pos == scr_ext.size())
            break;
    }

    // Flag.
    secret.push_back('@');
    secret.push_back('#');
    secret.push_back('$');
    secret.push_back('%');

    // Write secret file.
    pos = 0;
    while (iter != carry.cend()) {
        unsigned long i = static_cast<unsigned long>(secret[pos]);
        bitset<8> data{ i };
        for (size_t i = 0; i < 8; ++i)
        {
            if (findNextFrame(carry, ++iter)) {
                unsigned int bitrate = getBitrate(iter);
                if (bitrate < 13) {
                    if ((bitrate % 2) != data[i])
                        writeBitrate(++bitrate, iter);
                    continue;
                }
                else if (bitrate == 13)
                    writeBitrate(++bitrate, iter);
                --i;
            }
            else {
                cout << "Seek to the end.";
                return;
            }
        }
        ++pos;
        if (pos == secret.size())
            break;
    }

    // Output.
    WriteFile(carry, "output.mp3");
}

void Extract() {
    vector<char> carry;
    cout << "Carrier file: ";
    if (ReadFile(carry) == "")
        return;

    //Try syncing frames.
    int header = ((carry[6] & 0x7F) << 21) + ((carry[7] & 0x7F) << 14) + ((carry[8] & 0x7F) << 7) + (carry[9] & 0x7F) + 10;
    vector<char>::iterator iter = carry.begin() + header;
    findNextFrame(carry, iter);

    vector<unsigned int> brs;
    // Get file extension.
    string file_ext;
    while (iter != carry.cend()) {
        bitset<8> bs;
        for (size_t i = 0; i < 8; ++i) {
            if (findNextFrame(carry, ++iter)) {
                unsigned int br = getBitrate(iter);
                cout << getBitrate(iter) << endl;
                brs.push_back(br);
                if (br < 14)
                    bs[i] = br % 2;
                else
                    --i;
            }
            else {
                cout << "Seek to the end.";
                break;
            }

        }
        file_ext += static_cast<char>(bs.to_ulong());
        if (file_ext[file_ext.size() - 1] == '%')
            if (file_ext[file_ext.size() - 2] == '!')
                if (file_ext[file_ext.size() - 3] == '#')
                    if (file_ext[file_ext.size() - 4] == '&') {
                        file_ext.resize(file_ext.size() - 4);	// Delete flag.
                        break;
                    }

    }


    // Get secret file.
    vector<char> scr_file;
    while (iter != carry.cend()) {
        bitset<8> bs;
        for (size_t i = 0; i < 8; ++i) {
            findNextFrame(carry, ++iter);
            unsigned int br = getBitrate(iter);
            if (br < 14)
                bs[i] = br % 2;
            else
                --i;
        }
        scr_file.push_back(static_cast<char>(bs.to_ulong()));

        if (scr_file[scr_file.size() - 1] == '%')
            if (scr_file[scr_file.size() - 2] == '$')
                if (scr_file[scr_file.size() - 3] == '#')
                    if (scr_file[scr_file.size() - 4] == '@') {
                        scr_file.resize(scr_file.size() - 4);
                        WriteFile(scr_file, "secret" + (!file_ext.size() ? "" : "." + file_ext));
                        return;
                    }
    }
    cout << "ERROR: Nothing found." << endl;
}

string ReadFile(vector<char>& content) {
    char path[260];
    scanf("%s", path);
    FILE* pFile;
    errno_t error = fopen_s(&pFile, path, "rb");
    if (!error) {
        long fsize;
        fseek(pFile, 0, SEEK_END);
        fsize = ftell(pFile);
        rewind(pFile);
        content = vector<char>(fsize);
        fread(&content[0], 1, fsize, pFile);
        fclose(pFile);
        return path;
    }
    else {
        cout << "ERROR: Open file failed." << endl;
        return "";
    }

}

void WriteFile(vector<char>& content, string file_name) {
    FILE* output;
    errno_t error = fopen_s(&output, file_name.c_str(), "wb");
    if(error) {
        fwrite(static_cast<char*>(&content[0]), 1, content.size(), output);
        fclose(output);
        cout << "File has been saved as: " << file_name << endl;
    }
    else {
        cout << "ERROR: Open file failed." << endl;
        return;
    }
}

bool findNextFrame(vector<char> &carry, vector<char>::iterator &iter) {
    iter = find(iter, carry.end(), char(0xff));
    while (iter != carry.end()) {
        if (*(iter + 1) != char(0xfa) && *(iter + 1) != char(0xfb))
            iter = find(iter + 1, carry.end(), char(0xff));
        else
            break;
    }
    if (iter == carry.end())
        return false;
    return true;
}

unsigned int getBitrate(vector<char>::const_iterator iter) {
    iter += 2;
    unsigned long long i = static_cast<unsigned long long>(*iter);
    bitset<4> bs{ (i >> 4) };
    unsigned int bitrate = bs.to_ulong();
    return bitrate;
}

void writeBitrate(unsigned int bitrate, vector<char>::iterator iter) {
    iter += 2;
    unsigned long long i = static_cast<unsigned long long>(*iter);
    bitset<8> bs{ i };
    bitset<4> br{ bitrate };
    for (size_t i = 7; i != 3; --i)
        bs[i] = br[i - 4];
    char br_ch = static_cast<char>(bs.to_ulong());
    *iter = br_ch;
}

unsigned long long GetMP3Capacity(vector<char> &carry) {
    vector<char>::iterator iter = carry.begin();
    int count = 0;
    while (findNextFrame(carry, ++iter))
        if (getBitrate(iter) < 13)
            ++count;
    return count;
}
