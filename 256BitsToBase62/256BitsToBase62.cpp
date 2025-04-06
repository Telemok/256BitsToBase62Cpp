/**
 * @file    256BitsToBase62.cpp
 * @brief
 * convert 256 bits to ([0-9A-Za-z]){43} with speed 1M/sec (1 thread)
 * convert ([0-9A-Za-z]){43} => 256 bits with speed 1M/sec (1 thread)
 * this is 3-hours written algorithm, not library
 * @author  Dmitrii Arshinnikov <www.telemok.com@gmail.com> github.com/Telemok
 * https://github.com/Telemok/256BitsToBase62Cpp.git
 * @version 0.0.1
 * @date 2025-04-06
 *
 @verbatim

 Licensed under the Apache License, Version 2.0(the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 @endverbatim
 */

#include <iostream>
#include <fstream>
#include <string>
#include <windows.h> // Для ShellExecute
#include <iostream>
#include <cstdint>
#include <cstring>
#include <conio.h>
using namespace std;

#define FLAG_CALC_HASH_SYMBOL_SPREADING
long hashSymbolSpreadCounter[62];


/* Fast way to serialise any 256 bits to 43 bytes of [0-9A-Za-z] and return back. */
struct alignas(32) Data256 {
	uint64_t data[4];

	void setRandom()
	{
		for (int i = 0; i < 4; i++) {
			data[i] = 0;
			for (int j = 0; j < 4; j++) {
				data[i] = (data[i] << 16) | (rand() & 0xFFFF);
			}
		}
	}
	void clear()
	{
		memset(data, 0, sizeof(data));
	}
	Data256() {
		clear();
	}

	/** Convert 0-61 uint8 to [0-9A-Za-z]
	* No argument validation. */
	static char uint8_to_base62(uint8_t n) {
#ifdef FLAG_CALC_HASH_SYMBOL_SPREADING
		hashSymbolSpreadCounter[n]++;
#endif
		return n + (n < 10 ? '0' : n < 36 ? -10 + 'A' : -36 + 'a');
	}

	/** Convert [0-9A-Za-z] to 0-61 uint8
	* No argument validation. */
	static uint8_t base62_to_uint8(char c) {
		if ('0' <= c && c <= '9')
			return c - '0';

		if ('A' <= c && c <= 'Z')
			return c + (-'A' + 10);

		if ('a' <= c && c <= 'z')
			return c + (-'a' + 36);

		throw "base62_to_uint8";

		return 0;
	}

	/** Serialisation 4 * uint64_t to ([0-9A-Za-z]){43}.
	* No argument validation.
	* Can get speed up with AVX2 4*uint64_t,
	* can not use AVX2 8*uint32_t and 63-dimension not help. */
	void serialize(char* outBuffer43Chars09AZaz) {
		uint64_t sumOfRemainders = 0;

		/* Each uint64_t is (62 ^ 10) * 21.98. */
		for (int i = 0, power = 1; i < 4; i++, power *= 22) {
			uint64_t t = data[i];
			for (int j = 0; j < 10; j++) {
				*outBuffer43Chars09AZaz++ = uint8_to_base62(t % 62);
				t /= 62;
			}
			sumOfRemainders += t * power;
		}

		/* log(22 ^ 3, 62) = 2.996 => 3 symbols */
		for (int i = 0; i < 3; i++) {
			*outBuffer43Chars09AZaz++ = uint8_to_base62(sumOfRemainders % 62);
			sumOfRemainders /= 62;
		}
	}

	/** Deserialisation ([0-9A-Za-z]){43} to 4 * uint64_t.
	* No argument validation. */
	void deserialize(char* inBuffer43Chars09AZaz) {

		/* Unpack ([0-9A-Za-z]){3} to 4x22. */
		uint64_t sumOfRemainders = 0;
		for (int i = 2; i >= 0; i--) {
			sumOfRemainders = sumOfRemainders * 62 + base62_to_uint8(inBuffer43Chars09AZaz[40 + i]);
		}
		for (int i = 3, power = 22 * 22 * 22; i >= 0; i--, power /= 22) {
			data[i] = sumOfRemainders / power;
			sumOfRemainders %= power;
		}

		/* Unpack ([0-9A-Za-z]){40} to 4*(2^32)/21.98 */
		for (int i = 0; i < 4; i++) {
			for (int j = 9; j >= 0; j--) {
				data[i] = data[i] * 62 + base62_to_uint8(inBuffer43Chars09AZaz[i * 10 + j]);
			}
		}
	}

	bool operator==(const Data256& other) const {
		bool match = true;
		for (int i = 0; i < 4; i++) {
			if (data[i] != other.data[i]) {
				match = false;
				break;
			}
		}
		return match;
	}

};

int main() {

	for (int i = 0; i < 62; i++)
		hashSymbolSpreadCounter[i] = 0;

	for (int i = 0; i < 62; i++)
	{
		char c = Data256::uint8_to_base62(i);
		uint8_t d = Data256::base62_to_uint8(c);
		if (d != i)
		{
			throw "wrong symbol converter";
		}
	}

	for (int cycle = 0; cycle < 1000000; cycle++)
	{
		Data256 sourceData;
		sourceData.setRandom();
		char serialized[43];
		sourceData.serialize(serialized);

		Data256 destData;
		destData.deserialize(serialized);

		if (!(sourceData == destData))
		{
			cout << "Source 256 bits in hex:\n";
			for (int i = 0; i < 4; i++) {
				cout << hex << sourceData.data[i] << " ";
			}
			cout << dec << endl;

			cout << "Serialised ([0-9A-Za-z]){43}:\n";
			for (int i = 0; i < 43; i++) {
				cout << serialized[i];
			}
			cout << endl;

			cout << "Deserialised 256 bits in hex:\n";
			for (int i = 0; i < 4; i++) {
				cout << hex << destData.data[i] << " ";
			}
			cout << dec << endl;

			cout << "Data not match!: " << endl;


			int a = _getch();
		}
	}



	for (int i = 0; i < 62; i++)
		cout << hashSymbolSpreadCounter[i] << endl;


	cout << "If no error messages, program end OK: " << endl;


	std::string arrayStr;
	for (int i = 0; i < 62; i++) {
		arrayStr += std::to_string(hashSymbolSpreadCounter[i]);
		if (i < 61) arrayStr += ", ";
	}
	// HTML код как строка
	std::string htmlTemplate = R"(
<!DOCTYPE html>
<html>
<body>
<canvas id="myCanvas" width="620" height="400"></canvas>
<script>
var data=[];
const canvas = document.getElementById('myCanvas');
const ctx = canvas.getContext('2d');
const maxCount = Math.max(...data);
const barWidth = canvas.width / data.length;
for (let i = 0; i < data.length; i++) {
const barHeight = (data[i] / maxCount) * canvas.height;
ctx.fillRect(
i * barWidth,
canvas.height - barHeight,
barWidth - 1,
barHeight
);}
</script>
</body>
</html>
)";

	std::string result = htmlTemplate.substr(0, 107) + arrayStr + htmlTemplate.substr(107);

	// Сохраняем в файл
	std::ofstream outFile("result.html");
	if (!outFile) {
		std::cout << "Cannot create result.html" << std::endl;
		return 1;
	}

	outFile << result;
	outFile.close();

	std::cout << "File successfully created" << std::endl;

	// Открываем файл в браузере по умолчанию
	ShellExecuteA(NULL, "open", "result.html", NULL, NULL, SW_SHOWNORMAL);


	return 0;
}