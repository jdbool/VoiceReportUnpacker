#include <opus.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

struct Vector {
	float x;
	float y;
	float z;
};

static constexpr int errorUsage = 1;
static constexpr int errorFileIO = 2;
static constexpr int errorInvalidFileHeader = 3;
static constexpr int errorOpus = 4;

static constexpr int maxFrameSize = 960;
static constexpr size_t maxPacketSize = 2048;

static constexpr int sampleRate = 48000;

static constexpr size_t maxErrorLength = 1024;

static inline void printOpenError(std::filesystem::path& path) {
	char errorMessage[maxErrorLength];
	strerror_s(errorMessage, maxErrorLength, errno);

	std::cerr << "Failed to open file " << path << ": " << errorMessage << '\n';
}

static inline void writeWaveHeader(std::ofstream& output, uint16_t numFrames) {
	output.write("RIFF", 4);

	uint32_t chunkSize = 36 + (numFrames * maxFrameSize * sizeof(opus_int16));
	output.write(reinterpret_cast<char*>(&chunkSize), sizeof(chunkSize));

	output.write("WAVE", 4);
}

static inline void writeWaveFormatChunk(std::ofstream& output) {
	output.write("fmt ", 4);

	uint32_t formatChunkSize = 16;
	output.write(reinterpret_cast<char*>(&formatChunkSize),
	             sizeof(formatChunkSize));

	uint16_t audioFormat = 1;
	output.write(reinterpret_cast<char*>(&audioFormat), sizeof(audioFormat));

	uint16_t numChannels = 1;
	output.write(reinterpret_cast<char*>(&numChannels), sizeof(numChannels));

	uint32_t formatSampleRate = sampleRate;
	output.write(reinterpret_cast<char*>(&formatSampleRate),
	             sizeof(formatSampleRate));

	uint32_t byteRate = formatSampleRate * sizeof(opus_int16);
	output.write(reinterpret_cast<char*>(&byteRate), sizeof(byteRate));

	uint16_t blockAlign = sizeof(opus_int16);
	output.write(reinterpret_cast<char*>(&blockAlign), sizeof(blockAlign));

	uint16_t bitsPerSample = sizeof(opus_int16) * 8;
	output.write(reinterpret_cast<char*>(&bitsPerSample), sizeof(bitsPerSample));
}

static inline int convertFile(std::filesystem::path& path) {
	std::ifstream input(path, std::ifstream::binary);

	if (!input) {
		printOpenError(path);
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
		Vector position;
		input.read(reinterpret_cast<char*>(&position), sizeof(position));
		std::cout << "Position: (" << position.x << ", " << position.y << ", "
		          << position.z << ")\n";
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

	int err;
	OpusDecoder* decoder = opus_decoder_create(sampleRate, 1, &err);
	if (err < 0) {
		std::cerr << "Failed to create decoder: " << opus_strerror(err) << '\n';
		return errorOpus;
	}

	uint8_t frame[maxPacketSize];
	opus_int16 pcm[maxFrameSize];

	std::filesystem::path stem = path.stem();
	std::filesystem::create_directory(stem);

	for (int voiceIndex = 0; voiceIndex < numVoices; voiceIndex++) {
		std::cout << voiceIndex << '\n';

		uint32_t phoneNumber;
		input.read(reinterpret_cast<char*>(&phoneNumber), sizeof(phoneNumber));
		std::cout << " - Phone number: " << phoneNumber << '\n';

		Vector position;
		input.read(reinterpret_cast<char*>(&position), sizeof(position));
		std::cout << " - Position: (" << position.x << ", " << position.y << ", "
		          << position.z << ")\n";

		double distance;
		input.read(reinterpret_cast<char*>(&distance), sizeof(distance));
		std::cout << " - Distance: " << distance << '\n';

		uint16_t numFrames;
		input.read(reinterpret_cast<char*>(&numFrames), sizeof(numFrames));
		std::cout << " - Num frames: " << numFrames << '\n';

		std::filesystem::path voicePath =
		    stem / (std::to_string(phoneNumber) + '_' +
		            std::to_string((int)distance) + ".wav");

		std::cout << " - Output path: " << voicePath << '\n';

		std::ofstream output(voicePath, std::ofstream::binary);

		if (!output) {
			printOpenError(voicePath);
			return errorFileIO;
		}

		writeWaveHeader(output, numFrames);
		writeWaveFormatChunk(output);

		output.write("data", 4);

		{
			uint32_t dataSize = numFrames * maxFrameSize * sizeof(opus_int16);
			output.write(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
		}

		for (int frameIndex = 0; frameIndex < numFrames; frameIndex++) {
			uint8_t frameLength;
			input.read(reinterpret_cast<char*>(&frameLength), sizeof(frameLength));

			input.read(reinterpret_cast<char*>(&frame), frameLength);

			if (frameLength > 0) {
				int frameSize =
				    opus_decode(decoder, frame, frameLength, pcm, maxFrameSize, 0);
				if (frameSize < 0) {
					std::cerr << "Failed to decode frame: " << opus_strerror(err) << '\n';
					return errorOpus;
				} else if (frameSize != maxFrameSize) {
					std::cerr << "Decoder returned invalid frame size\n";
					return errorOpus;
				}
			} else {
				std::memset(pcm, 0, maxFrameSize * sizeof(opus_int16));
			}

			output.write(reinterpret_cast<const char*>(pcm),
			             maxFrameSize * sizeof(opus_int16));
		}
	}

	opus_decoder_destroy(decoder);

	return 0;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " inputFilePath.vcreport\n";
		return errorUsage;
	}

	std::filesystem::path path(argv[1]);
	return convertFile(path);
}