
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
/*
-----------------------------------
FIRST ORDER SCHEMA
-----------------------------------
TWO ROWS VERSION
Row A   01000000
Row B   00100000
-----------------------------------
        01000000
XOR  ⊕ 00100000
-----------------------------------
      = 01100000 (XOR Value)
-----------------------------------
ONE ROW VERSION
Row {A, B} = {00},{10},{01},{00},{00},{00},{00},{00}
-----------------------------------
SECOND ORDER SCHEMA
-----------------------------------
XOR TABLE
Set     Bit        XOR          Derived     Order
        Order      Value        Key if...   (if {A, B} else {B, A})
-------------------------------------------------------------------
Set 1   00         = 0          => 0        {B, A}
Set 1   10         = 1          => 1        {A, B}
Set 2   01         = 1          => 0        {B, A}
Set 2   11         = 0          => 1        {A, B}
-----------------------------------
*/
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
// Header Files
//----------------------------------------------------------------------------------

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <mutex>
#include <deque>
#include <string>
#include <utility>
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <cstdint>
#include <cctype>
#include <bitset>
#include <fstream>
#include <limits>
#include <unordered_map>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

class SystemClock {
  public:
    inline long long getNanoseconds() {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    }
};

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

class Functions {
  public:
    size_t nextPowerOf2(size_t n) {
        size_t p = 1;
        while (p < n)
            p <<= 1;
        return p;
    }

    std::string padToPowerOf2(std::string binary) {
        size_t target = nextPowerOf2(binary.size());
        binary.resize(target, '0');
        return binary;
    }

    std::string stringToBinaryASCII(const std::string &input) {
        std::string binary;
        binary.reserve(input.size() * 8);

        for (char c : input) {
            std::bitset<8> bits(static_cast<unsigned char>(c));
            binary += bits.to_string();
        }

        while ((binary.size() & (binary.size() - 1)) != 0) {
            binary.push_back('0');
        }

        return binary;
    }

    std::string binaryASCIIToString(const std::string &binary) {
        if (binary.size() % 8 != 0) {
            throw std::runtime_error("Binary length must be multiple of 8");
        }

        std::string output;
        output.reserve(binary.size() / 8);

        for (size_t i = 0; i < binary.size(); i += 8) {
            std::bitset<8> bits(binary.substr(i, 8));
            output.push_back(static_cast<char>(bits.to_ulong()));
        }

        return output;
    }

    // ----------------------------
    // NEW: bytes → binary string
    // ----------------------------
    std::string bytesToBinary(const std::vector<uint8_t> &bytes) {
        std::string binary;
        binary.reserve(bytes.size() * 8);

        for (uint8_t b : bytes) {
            for (int i = 0; i < 8; i++) {
                binary.push_back(((b >> i) & 1) ? '1' : '0');
            }
        }

        return binary;
    }

    // ----------------------------
    // NEW: binary string → bytes
    // ----------------------------
    std::vector<uint8_t> binaryToBytes(const std::string &binary) {
        if (binary.size() % 8 != 0) {
            throw std::runtime_error("Binary size must be multiple of 8");
        }

        std::vector<uint8_t> bytes(binary.size() / 8);

        for (size_t i = 0; i < bytes.size(); i++) {
            uint8_t b = 0;

            for (int j = 0; j < 8; j++) {
                if (binary[i * 8 + j] == '1')
                    b |= (1 << j);
            }

            bytes[i] = b;
        }

        return bytes;
    }
};

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

namespace CRYPTO {
class SHA256 {
  public:
    SHA256() { reset(); }

    void update(const uint8_t *data, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            buffer[bufferLen++] = data[i];
            if (bufferLen == 64) {
                transform(buffer);
                bitlen += 512;
                bufferLen = 0;
            }
        }
    }

    void update(const std::string &data) { update(reinterpret_cast<const uint8_t *>(data.c_str()), data.size()); }

    std::string digest() {
        uint64_t totalBits = bitlen + bufferLen * 8;

        buffer[bufferLen++] = 0x80;
        if (bufferLen > 56) {
            while (bufferLen < 64)
                buffer[bufferLen++] = 0x00;
            transform(buffer);
            bufferLen = 0;
        }

        while (bufferLen < 56)
            buffer[bufferLen++] = 0x00;

        for (int i = 7; i >= 0; --i)
            buffer[bufferLen++] = (totalBits >> (i * 8)) & 0xFF;

        transform(buffer);

        std::ostringstream oss;
        for (int i = 0; i < 8; ++i)
            oss << std::hex << std::setw(8) << std::setfill('0') << h[i];

        reset(); // reset internal state after digest
        return oss.str();
    }

    std::string digestBinary() {
        std::string hex = digest();
        std::string binary;
        for (char c : hex) {
            uint8_t val = (c <= '9') ? c - '0' : 10 + (std::tolower(c) - 'a');
            for (int i = 3; i >= 0; --i)
                binary += ((val >> i) & 1) ? '1' : '0';
        }
        return binary;
    }

    void reset() {
        h[0] = 0x6a09e667;
        h[1] = 0xbb67ae85;
        h[2] = 0x3c6ef372;
        h[3] = 0xa54ff53a;
        h[4] = 0x510e527f;
        h[5] = 0x9b05688c;
        h[6] = 0x1f83d9ab;
        h[7] = 0x5be0cd19;
        bitlen = 0;
        bufferLen = 0;
    }

  private:
    uint32_t h[8];
    uint64_t bitlen;
    uint8_t buffer[64];
    size_t bufferLen;

    void transform(const uint8_t block[64]) {
        uint32_t w[64];

        for (int i = 0; i < 16; ++i) {
            w[i] = (block[i * 4] << 24) | (block[i * 4 + 1] << 16) | (block[i * 4 + 2] << 8) | (block[i * 4 + 3]);
        }

        for (int i = 16; i < 64; ++i) {
            w[i] = theta1(w[i - 2]) + w[i - 7] + theta0(w[i - 15]) + w[i - 16];
        }

        uint32_t a = h[0];
        uint32_t b = h[1];
        uint32_t c = h[2];
        uint32_t d = h[3];
        uint32_t e = h[4];
        uint32_t f = h[5];
        uint32_t g = h[6];
        uint32_t h_val = h[7];

        for (int i = 0; i < 64; ++i) {
            uint32_t temp1 = h_val + sig1(e) + choose(e, f, g) + K[i] + w[i];
            uint32_t temp2 = sig0(a) + majority(a, b, c);
            h_val = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
        h[5] += f;
        h[6] += g;
        h[7] += h_val;
    }

    static uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
    static uint32_t choose(uint32_t e, uint32_t f, uint32_t g) { return (e & f) ^ (~e & g); }
    static uint32_t majority(uint32_t a, uint32_t b, uint32_t c) { return (a & b) ^ (a & c) ^ (b & c); }
    static uint32_t sig0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
    static uint32_t sig1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
    static uint32_t theta0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
    static uint32_t theta1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

    const uint32_t K[64] = {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be,
                            0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa,
                            0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85,
                            0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
                            0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
                            0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
};
} // namespace CRYPTO

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

class RandomNumberGenerator {
  public:
    inline std::string run() {
        std::string result;
        result.reserve((totalIterations - localBufferSize) * 256);

        for (int i = 0; i < totalIterations; ++i) {

            long long duration = countdown();
            ++count;
            globalSum += duration;
            globalAvg = globalSum / count;

            int bit = duration < globalAvg ? 0 : 1;

            if (localBits.size() >= localBufferSize)
                localBits.pop_front();

            localBits.push_back(bit);

            if (localBits.size() == localBufferSize) {
                // 32 raw bytes → 256 bit string
                std::string hashBits = hashLocalBits();
                result += hashBits;
            }
        }

        return result;
    }

  private:
    CRYPTO::SHA256 sha;
    SystemClock systemClock;
    std::deque<int> localBits;
    const int totalIterations = 1000;
    const size_t localBufferSize = 512;
    long long globalSum = 0;
    long long globalAvg = 0;
    int count = 0;

    inline long long countdown() {
        int x = 10;
        auto start = systemClock.getNanoseconds();
        while (x > 0)
            x--;
        auto end = systemClock.getNanoseconds();
        return end - start;
    }

    inline std::string hashLocalBits() {
        // Build 64-byte block
        uint8_t bytes[64] = {0};
        for (size_t i = 0; i < localBits.size(); ++i) {
            if (localBits[i]) {
                bytes[i / 8] |= (1 << (7 - (i % 8)));
            }
        }

        sha.update(bytes, 64);

        // Return 256-bit binary string using fast helper
        return sha.digestBinary();
    }
};

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

class BinaryEntropyPool {
  public:
    inline std::string get(size_t bitsNeeded) {
        std::lock_guard<std::mutex> lock(poolMutex);

        // Refill the pool until we have enough bits
        while (bitPool.size() < bitsNeeded) {
            bitPool += rng.run(); // rng.run() now returns a bit string
        }

        // Extract exactly the number of bits requested
        std::string result = bitPool.substr(0, bitsNeeded);
        bitPool.erase(0, bitsNeeded); // remove consumed bits

        return result;
    }

  private:
    std::string bitPool; // bit string directly
    RandomNumberGenerator rng;
    mutable std::mutex poolMutex;
};

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

class XORCompress {
  public:
    struct Result {
        std::string key;
        std::string xored;
    };

    struct SymbolEntry {
        uint16_t wordId;
        std::vector<uint16_t> positions;
    };

    struct EncodingMeta {
        int wordIdBytes;
        int positionBytes;
    };

    inline Result compress(const std::string &data) {
        if (data.size() < 2) {
            throw std::runtime_error("Too small for XOR compression");
        }

        std::string xored;
        std::string key;

        xored.reserve(data.size() / 2);
        key.reserve(data.size() / 2);

        for (std::size_t i = 0; i < data.size(); i += 2) {
            char A = data[i];
            char B = data[i + 1];

            if ((A != '0' && A != '1') || (B != '0' && B != '1')) {
                throw std::runtime_error("Invalid bit");
            }

            xored.push_back(A == B ? '0' : '1');
            key.push_back(A);
        }

        return {key, xored};
    }

    inline std::string decompress(const Result &result) {
        const std::string &key = result.key;
        const std::string &xored = result.xored;

        if (key.size() != xored.size()) {
            throw std::runtime_error("Invalid structure");
        }

        std::string data;
        data.reserve(key.size() * 2);

        for (std::size_t i = 0; i < key.size(); ++i) {
            char K = key[i];
            char R = xored[i];

            if ((K != '0' && K != '1') || (R != '0' && R != '1')) {
                throw std::runtime_error("Invalid bit");
            }

            char A = K;
            char B = (K == R ? '0' : '1');

            data.push_back(A);
            data.push_back(B);
        }

        return data;
    }

    std::vector<SymbolEntry> encodeWords(const std::string &text) {

        idToWord.clear();

        std::unordered_map<std::string, uint16_t> wordToId;
        std::unordered_map<std::string, std::vector<uint16_t>> positions;

        uint16_t nextId = 1;
        uint16_t pos = 0;

        std::vector<std::string> tokens;
        std::string current;

        for (char c : text) {

            if (c == ' ' || c == '\n' || c == '\t' || c == '\r') {

                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }

                tokens.push_back(std::string(1, c));

            } else {
                current.push_back(c);
            }
        }

        if (!current.empty())
            tokens.push_back(current);

        // --------------------------
        // BUILD INDEX
        // --------------------------
        for (const std::string &w : tokens) {

            pos++;

            if (!wordToId.count(w)) {
                wordToId[w] = nextId;
                idToWord[nextId] = w;
                nextId++;
            }

            uint16_t id = wordToId[w];
            positions[w].push_back(pos);
        }

        // --------------------------
        // BUILD OUTPUT
        // --------------------------
        std::vector<SymbolEntry> output(wordToId.size());

        for (auto &[w, id] : wordToId) {
            output[id - 1] = {id, positions[w]};
        }

        return output;
    }

    // --------------------------
    // REBUILD ORIGINAL TEXT
    // --------------------------
    std::string decodeToTextExact(const std::vector<SymbolEntry> &data) {

        std::vector<std::pair<uint16_t, std::string>> ordered;

        for (const auto &e : data) {

            std::string word = idToWord[e.wordId];

            for (auto p : e.positions) {
                ordered.push_back({p, word});
            }
        }

        std::sort(ordered.begin(), ordered.end(), [](auto &a, auto &b) { return a.first < b.first; });

        std::string result;

        for (auto &x : ordered) {
            result += x.second;
        }

        return result;
    }

    // --------------------------
    // PRINT STRUCTURE
    // --------------------------
    void printEncoded(const std::vector<SymbolEntry> &data) {

        size_t totalPositions = 0;

        std::cout << "\n--- Encoded Index ---\n";

        for (const auto &entry : data) {

            totalPositions += entry.positions.size();

            std::cout << "WordID: " << entry.wordId << " | Occ: " << entry.positions.size() << " | Positions: ";

            for (size_t i = 0; i < entry.positions.size(); i++) {
                std::cout << entry.positions[i];
                if (i + 1 < entry.positions.size())
                    std::cout << ",";
            }

            std::cout << "\n";
        }

        std::cout << "\nTotal positions: " << totalPositions << "\n";
    }

    // --------------------------
    // BYTE STREAM ENCODING
    // --------------------------
    std::vector<uint8_t> encodeToBytes(const std::vector<SymbolEntry> &data, EncodingMeta meta) {

        std::vector<uint8_t> out;

        auto pushInt = [&](uint16_t v, int bytes) {
            for (int i = 0; i < bytes; i++)
                out.push_back((v >> (8 * i)) & 0xFF);
        };

        out.push_back(meta.wordIdBytes);
        out.push_back(meta.positionBytes);

        pushInt((uint16_t)data.size(), 2);

        for (const auto &e : data) {

            pushInt(e.wordId, meta.wordIdBytes);
            pushInt((uint16_t)e.positions.size(), meta.positionBytes);

            for (auto p : e.positions)
                pushInt(p, meta.positionBytes);
        }

        return out;
    }

    std::vector<SymbolEntry> decodeFromBytes(const std::vector<uint8_t> &data) {

        if (data.size() < 3)
            throw std::runtime_error("Corrupt byte stream");

        size_t idx = 0;

        EncodingMeta meta;
        meta.wordIdBytes = data[idx++];
        meta.positionBytes = data[idx++];

        auto readInt = [&](int bytes) -> uint16_t {
            uint16_t value = 0;
            for (int i = 0; i < bytes; i++) {
                if (idx >= data.size())
                    throw std::runtime_error("Unexpected EOF");

                value |= (data[idx++] << (8 * i));
            }
            return value;
        };

        uint16_t entryCount = readInt(2);

        std::vector<SymbolEntry> output;
        output.reserve(entryCount);

        for (uint16_t i = 0; i < entryCount; i++) {

            SymbolEntry e;

            e.wordId = readInt(meta.wordIdBytes);

            uint16_t posCount = readInt(meta.positionBytes);

            e.positions.reserve(posCount);

            for (uint16_t j = 0; j < posCount; j++) {
                e.positions.push_back(readInt(meta.positionBytes));
            }

            output.push_back(std::move(e));
        }

        return output;
    }

    // --------------------------
    // PRINT RAW BINARY
    // --------------------------
    void printByteStream(const std::vector<uint8_t> &data) {

        std::cout << "\nTotal bytes: " << data.size() << "\n";
        std::cout << "--- RAW 8-BIT STREAM ---\n";

        for (size_t i = 0; i < data.size(); i++) {

            // start of a new 8-byte line
            if (i % 8 == 0) {
                std::cout << "\n[" << (i / 8) << "] ";
            }

            std::cout << std::bitset<8>(data[i]) << " ";
        }

        std::cout << "\n----------------------\n";
    }

    // --------------------------
    // META
    // --------------------------
    EncodingMeta computeMeta(const std::vector<SymbolEntry> &data) {
        uint16_t maxId = 0, maxPos = 0;

        for (const auto &e : data) {
            maxId = std::max(maxId, e.wordId);

            for (auto p : e.positions)
                maxPos = std::max(maxPos, p);
        }

        return {bytesNeeded(maxId), bytesNeeded(maxPos)};
    }

    inline void writeToFile(const std::string &filename, const XORCompress::Result &r) {
        std::ofstream out(filename);
        if (!out)
            throw std::runtime_error("Failed to open file");

        out << "==================================================\n";
        out << "                 XOR COMPRESSOR\n";
        out << "==================================================\n\n";
        out << "KEY\n";
        out << r.key << "\n";
        out << "\n==================================================\n";
        out << "XOR VALUE\n";
        out << r.xored << "\n";
        out << "\n==================================================\n";
    }

    inline XORCompress::Result readFromFile(const std::string &filename) {
        std::ifstream in(filename);
        if (!in)
            throw std::runtime_error("Failed to open file");

        XORCompress::Result r;
        std::string line;

        enum class Section { NONE, KEY, XOR };
        Section section = Section::NONE;

        while (std::getline(in, line)) {

            // ignore separators
            if (line.find("====") != std::string::npos)
                continue;

            // --------------------------
            // FIX: flexible matching
            // --------------------------
            if (line.find("KEY") != std::string::npos) {
                section = Section::KEY;
                continue;
            }

            if (line.find("XOR") != std::string::npos) {
                section = Section::XOR;
                continue;
            }

            if (line.empty())
                continue;

            if (section == Section::KEY && r.key.empty()) {
                r.key = line;
            } else if (section == Section::XOR && r.xored.empty()) {
                r.xored = line;
            }
        }

        // validation
        if (r.key.empty())
            throw std::runtime_error("Missing KEY");

        if (r.xored.empty())
            throw std::runtime_error("Missing XOR");

        if (r.key.size() != r.xored.size())
            throw std::runtime_error("Corrupt file (size mismatch)");

        return r;
    }

  private:
    std::unordered_map<uint16_t, std::string> idToWord;

    int bytesNeeded(uint16_t v) {
        if (v <= 0xFF)
            return 1;
        if (v <= 0xFFFF)
            return 2;
        return 4;
    }
};

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

class UserInterface {
  public:
    void run() {

        std::string text = getInputText();

        std::cout << "\nORIGINAL:\n" << text << "\n";

        if (!askYesNo("Compress data? (y/n): "))
            return;

        compressPipeline(text);

        if (!askYesNo("\nDecompress data? (y/n): "))
            return;

        decompressPipeline(text);
    }

  private:
    XORCompress compressor;
    FileSystem fileSystem;
    Functions utils;

    std::string fileName = "compressed.txt";

    // ----------------------------
    std::string getInputText() {
        return "If you can keep your head when all about you\n"
               "    Are losing theirs and blaming it on you,\n"
               "If you can trust yourself when all men doubt you,\n"
               "    But make allowance for their doubting too;\n"
               "If you can wait and not be tired by waiting,\n"
               "    Or being lied about, don’t deal in lies,\n"
               "Or being hated, don’t give way to hating,\n"
               "    And yet don’t look too good, nor talk too wise:\n"
               "\n"
               "If you can dream—and not make dreams your master;\n"
               "    If you can think—and not make thoughts your aim;\n"
               "If you can meet with Triumph and Disaster\n"
               "    And treat those two impostors just the same;\n"
               "If you can bear to hear the truth you’ve spoken\n"
               "    Twisted by knaves to make a trap for fools,\n"
               "Or watch the things you gave your life to, broken,\n"
               "    And stoop and build ’em up with worn-out tools:\n"
               "\n"
               "If you can make one heap of all your winnings\n"
               "    And risk it on one turn of pitch-and-toss,\n"
               "And lose, and start again at your beginnings\n"
               "    And never breathe a word about your loss;\n"
               "If you can force your heart and nerve and sinew\n"
               "    To serve your turn long after they are gone,\n"
               "And so hold on when there is nothing in you\n"
               "    Except the Will which says to them: ‘Hold on!’\n"
               "\n"
               "If you can talk with crowds and keep your virtue,\n"
               "    Or walk with Kings—nor lose the common touch,\n"
               "If neither foes nor loving friends can hurt you,\n"
               "    If all men count with you, but none too much;\n"
               "If you can fill the unforgiving minute\n"
               "    With sixty seconds’ worth of distance run,\n"
               "Yours is the Earth and everything that’s in it,\n"
               "    And—which is more—you’ll be a Man, my son!\n";
    }

    // ----------------------------
    void compressPipeline(const std::string &text) {

        auto encoded = compressor.encodeWords(text);
        auto meta = compressor.computeMeta(encoded);

        auto bytes = compressor.encodeToBytes(encoded, meta);

        std::string binary = utils.bytesToBinary(bytes);
        binary = utils.padToPowerOf2(binary);

        auto result = compressor.compress(binary);

        compressor.writeToFile(fileName, result);

        std::cout << "\n--- COMPRESSED ---\n";
        std::cout << "Key size: " << result.key.size() << "\n";
        std::cout << "XOR size: " << result.xored.size() << "\n";
    }

    // ----------------------------
    void decompressPipeline(const std::string &originalText) {

        std::cout << "\nReading file...\n";

        auto loaded = compressor.readFromFile(fileName);

        // STEP 1: XOR decompress → binary string
        std::string restoredBinary = compressor.decompress(loaded);

        std::cout << "\nRestored binary size: " << restoredBinary.size() << "\n";

        // STEP 2: binary → bytes (ONLY ONCE)
        std::vector<uint8_t> bytes = utils.binaryToBytes(restoredBinary);

        // STEP 3: bytes → symbol structure
        auto symbols = compressor.decodeFromBytes(bytes);

        // STEP 4: symbols → text
        std::string text = compressor.decodeToTextExact(symbols);

        std::cout << "\n--- RESTORED TEXT ---\n";
        std::cout << text << "\n";

        (void)originalText;
    }

    // ----------------------------
    bool askYesNo(const std::string &msg) {
        std::cout << msg;
        char c;
        std::cin >> c;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return (c == 'y' || c == 'Y');
    }
};

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::cout << "Welcome to the Program...\n";
    std::cout << "\nPress Enter to continue...\n";
    std::cin.get();

    try {
        UserInterface ui;
        ui.run();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
    }

    std::cout << "\nPress Enter to exit...";
    std::cin.get();

    return 0;
}

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
