#ifndef UTIL_HPP
#define UTIL_HPP

#include <iostream>
#include <map>
#include <chrono>
#include <ctime>

namespace Util {
	void log(std::string message, bool err = false) {
		auto now = std::chrono::system_clock::now();
		std::time_t now_c = std::chrono::system_clock::to_time_t(now);
		struct tm* parts = std::localtime(&now_c);
		std::string strHr = std::to_string(parts->tm_hour);
		std::string strMin = std::to_string(parts->tm_min);
		std::string strSec = std::to_string(parts->tm_sec);

		if (strHr.length() != 2) {
			strHr = "0" + strHr;
		}
		if (strMin.length() != 2) {
			strMin = "0" + strMin;
		}
		if (strSec.length() != 2) {
			strSec = "0" + strSec;
		}

		std::string timestamp = "[";
		timestamp += strHr;
		timestamp += ":";
		timestamp += strMin;
		timestamp += ":";
		timestamp += strSec;
		timestamp += "] ";

		if (err) {
			std::cerr << timestamp << message << std::endl;
		} else {
			std::cout << timestamp << message << std::endl;
		}
	}

	static const std::string base64_chars =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";

	static std::string base64encode(const std::string& input) {
		std::string encoded;
		int val = 0, valb = -6;

		for (unsigned char c : input) {
			val = (val << 8) + c;
			valb += 8;
			while (valb >= 0) {
				encoded.push_back(base64_chars[(val >> valb) & 0x3F]);
				valb -= 6;
			}
		}

		if (valb > -6) {
			encoded.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
		}

		while (encoded.size() % 4) {
			encoded.push_back('=');
		}

		return encoded;
	}

	const std::map<int, char> unicodeToAscii = {
		{0x0104, 'A'},  // •
		{0x0118, 'E'},  //  
		{0x0106, 'C'},  // ∆
		{0x015A, 's'},  // å
		{0x0179, 'Z'},  // Ø
		{0x017B, 'Z'},  // è
		{0x0143, 'N'},  // —
		{0x0141, 'L'},  // £
		{0x00D3, 'O'},  // ”
		{0x0105, 'a'},  // π
		{0x0119, 'e'},  // Í
		{0x0107, 'c'},  // Ê
		{0x015B, 's'},  // ú
		{0x017A, 'z'},  // ø
		{0x017C, 'z'},  // ü
		{0x0144, 'n'},  // Ò
		{0x0142, 'l'},  // ≥
		{0x00F3, 'o'},  // Û
	};

	std::string plToEn(const std::string& utf8Input) {
		std::string result;

		for (size_t i = 0; i < utf8Input.length(); ) {
			unsigned char current = utf8Input[i];

			if (current < 0x80) {
				result += current;
				++i;
			} else {
				int codepoint = 0;

				if ((current & 0xE0) == 0xC0) {
					codepoint = (current & 0x1F) << 6 | (utf8Input[i + 1] & 0x3F);
					i += 2;
				} else if ((current & 0xF0) == 0xE0) {
					codepoint = (current & 0x0F) << 12 | (utf8Input[i + 1] & 0x3F) << 6 | (utf8Input[i + 2] & 0x3F);
					i += 3;
				} else if ((current & 0xF8) == 0xF0) {
					codepoint = (current & 0x07) << 18 | (utf8Input[i + 1] & 0x3F) << 12 | (utf8Input[i + 2] & 0x3F) << 6 | (utf8Input[i + 3] & 0x3F);
					i += 4;
				}

				auto it = unicodeToAscii.find(codepoint);
				if (it != unicodeToAscii.end()) {
					result += it->second;
				} else {
					result += utf8Input.substr(i - (current < 0xE0 ? 1 : 2), (current < 0xE0 ? 1 : 2) + (current < 0xE0 ? 0 : 1));
				}
			}
		}

		return result;
	}
}

#endif