#include <fstream>
#include <iostream>
#include <string>

static constexpr int errorUsage = 1;
static constexpr int errorFileIO = 2;
static constexpr int errorInvalidFileHeader = 3;

static constexpr size_t maxFrameSize = 2048;

static inline int convertFile(const char* filePath) {
	std::ifstream input(filePath, std::ifstream::binary);

	if (!input) {
		std::cerr << "Failed to open file " << filePath << ": " << strerror(errno)
		          << '\n';
		return errorFileIO;
	}

	{
		char magic[5] = {0};
		input.read(magic, 4);
		if (std::strcmp(magic, "sVCr") != 0) {
			std::cerr << "Invalid file header\n";
			return errorInvalidFileHeader;
		}
	}

	{
		uint32_t phoneNumber;
		input.read(reinterpret_cast<char*>(&phoneNumber), sizeof(phoneNumber));
		std::cout << "Phone number: " << phoneNumber << '\n';
	}

	{
		uint8_t reasonLength;
		input.read(reinterpret_cast<char*>(&reasonLength), sizeof(reasonLength));
		std::cout << "Reason length: " << (int)reasonLength << '\n';

		std::string reason(reasonLength, '\0');
		input.read(reinterpret_cast<char*>(reason.data()), reasonLength);
		std::cout << "Reason: " << reason << '\n';
	}

	uint8_t numVoices;
	input.read(reinterpret_cast<char*>(&numVoices), sizeof(numVoices));
	std::cout << "Num voices: " << (int)numVoices << '\n';

	uint8_t frame[maxFrameSize];

	for (int voiceIndex = 0; voiceIndex < numVoices; voiceIndex++) {
		std::cout << voiceIndex << '\n';

		uint32_t phoneNumber;
		input.read(reinterpret_cast<char*>(&phoneNumber), sizeof(phoneNumber));
		std::cout << " - Phone number: " << phoneNumber << '\n';

		uint16_t numFrames;
		input.read(reinterpret_cast<char*>(&numFrames), sizeof(numFrames));
		std::cout << " - Num frames: " << numFrames << '\n';

		for (int frameIndex = 0; frameIndex < numFrames; frameIndex++) {
			uint8_t frameLength;
			input.read(reinterpret_cast<char*>(&frameLength), sizeof(frameLength));
			std::cout << " - Frame length: " << (int)frameLength << '\n';

			input.read(reinterpret_cast<char*>(&frame), frameLength);
		}
	}

	return 0;
}

int main(int argc, char** argv) {
	std::cout << "Hello, world!\n";

	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " inputFilePath.vcreport\n";
		return errorUsage;
	}

	return convertFile(argv[1]);
}