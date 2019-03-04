// Hider.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <bitset>

using namespace std;


void Encode();
void Extract();

string ReadFile(vector<char>& content);
void WriteFile(vector<char>& content, string file_name);

bool findNextFrame(vector<char> &carry, vector<char>::iterator &iter);
unsigned int getBitrate(vector<char>::const_iterator iter);
void writeBitrate(unsigned int bitrate, vector<char>::iterator iter);
int GetMP3Capacity(vector<char> &carry);


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
		cout << "What to do? ==> ";
	}
	//Encode();
}

void Encode() {
	vector<char> carry;
	vector<char> secret;

	cout << "Carrier file: ";
	if (ReadFile(carry) == "")
		return;
	//int capacity = GetMP3Capacity(carry);
	//cout << "The capacity of the carrier file is " << capacity << endl;

	//Try syncing frames.
	int header = ((carry[6] & 0x7F) << 21) + ((carry[7] & 0x7F) << 14) + ((carry[8] & 0x7F) << 7) + (carry[9] & 0x7F) + 10;
	vector<char>::iterator iter = carry.begin() + header;

	cout << "Secret file: ";
	string scr_ext,
		scr_path = ReadFile(secret);
	if (scr_path == "")
		return;

	// Get secret file extension.
	for (size_t i = scr_path.size() - 1; i > 0; --i) {
		if (scr_path[i] != '.')
			scr_ext = scr_path[i] + scr_ext;
		else
			break;
	}

	int pos = 0;
	// Skip the first frame.
	findNextFrame(carry, iter);

	// Write file extension.
	scr_ext += "&#!%";
	while (iter != carry.cend()) {
		unsigned int hd = scr_ext[pos];
		bitset<8> header{ hd };
		for (int i = 0; i < 8; ++i) {
			if (findNextFrame(carry, ++iter)) {
				unsigned int bitrate = getBitrate(iter);
				if (bitrate < 13) {
					if ((bitrate % 2) != header[i])
						writeBitrate(++bitrate, iter);
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
		unsigned int i = secret[pos];
		bitset<8> data{ i };
		for (int i = 0; i < 8; ++i)
		{
			if (findNextFrame(carry, ++iter)) {
				int bitrate = getBitrate(iter);
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

	// Get file extension.
	string file_ext;
	while (iter != carry.cend()) {
		bitset<8> bs;
		for (int i = 0; i < 8; ++i) {
			if (findNextFrame(carry, ++iter)) {
				unsigned int br = getBitrate(iter);
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
		for (int i = 0; i < 8; ++i) {
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
	//const char* path = "C:\\Users\\N1tri\\Music\\2.mp3";
	char path[260];
	scanf_s("%s", path, 260);
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
	fwrite((char*)&content[0], 1, content.size(), output);
	fclose(output);
	cout << "File has been saved as: " << file_name << endl;
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
	unsigned __int64 i = *iter;
	bitset<4> bs{ (i >> 4) };
	unsigned int bitrate = bs.to_ulong();
	return bitrate;
}

void writeBitrate(unsigned int bitrate, vector<char>::iterator iter) {
	iter += 2;
	unsigned __int64 i = *iter;
	bitset<8> bs{ i };
	bitset<4> br{ bitrate };
	for (int i = 7; i != 3; --i)
		bs[i] = br[i - 4];
	char br_ch = static_cast<char>(bs.to_ulong());
	*iter = br_ch;
}

int GetMP3Capacity(vector<char> &carry) {
	auto iter = carry.begin();
	int count = 0;
	while (findNextFrame(carry, iter)) {
		++iter;
		++count;
	}
	return count;
}