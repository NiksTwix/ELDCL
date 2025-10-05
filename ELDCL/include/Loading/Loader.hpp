#pragma once
#include "..\Decoding\Decoder.hpp"
#include <fstream>

namespace DCL 
{
    class Loader {
    public:
        static std::shared_ptr<ContainersTree> LoadFromFile(const std::string& filename) {
            std::ifstream file(filename);
            if (!file.is_open()) {
                std::cout << "Failed to open: " << filename << "\n";
                return nullptr;
            }

            std::string content((std::istreambuf_iterator<char>(file)),
                std::istreambuf_iterator<char>());

            Lexer lexer;
            auto tokens = lexer.ToTokens(content);

            Decoder decoder;
            return decoder.Decode(tokens);
        }

        static std::shared_ptr<ContainersTree> LoadFromString(const std::string& content) {
            Lexer lexer;
            auto tokens = lexer.ToTokens(content);

            Decoder decoder;
            return decoder.Decode(tokens);
        }
    };
}
