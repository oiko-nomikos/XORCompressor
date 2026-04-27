
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

class FileSystem {
  public:
    static void writeToFile(const std::string &filename, const std::string &content) {
        std::ofstream out(filename);
        if (!out)
            throw std::runtime_error("Failed to open file");
        out << content;
    }

    static std::string readFromFile(const std::string &filename) {
        std::ifstream in(filename);
        if (!in)
            throw std::runtime_error("Failed to open file");
        std::stringstream buffer;
        buffer << in.rdbuf();
        return buffer.str();
    }
};

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------

class XORCompressor {
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

            // uint16_t id = wordToId[w];
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

    inline void writeToFile(const std::string &filename, const XORCompressor::Result &r) {
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

    inline XORCompressor::Result readFromFile(const std::string &filename) {
        std::ifstream in(filename);
        if (!in)
            throw std::runtime_error("Failed to open file");

        XORCompressor::Result r;
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

    int bytesNeeded(uint32_t v) {
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
    XORCompressor compressor;
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

        compressor.printEncoded(encoded);

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
