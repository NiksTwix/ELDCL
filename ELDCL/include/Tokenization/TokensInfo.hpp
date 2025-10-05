#pragma once
#include "..\Definitions\Types.hpp"



namespace DCL 
{
    enum TokenType {
        UDE = 0,        // 00000000 // Undefined
        IDENTIFIER = 1 << 0,   // 00000001  // SContainer, Key
        OPERATOR = 1 << 1,   // 00000010   // + - * / :: 
        NUMBER_LITERAL = 1 << 2,  // 00000100 // Numbers, strings, true/false
        STRING_LITERAL = 1 << 3,  // 00001000 
        BOOL_LITERAL = 1 << 4,   // 00010000
        DELIMITER = 1 << 5,   // 00100000 //, {}, [],
        END = 1 << 6,   // 01000000 // ;
        KEYWORD = 1 << 7,    // 10000000 //tag
        LITERALS = NUMBER_LITERAL | STRING_LITERAL | BOOL_LITERAL,
        SYNTAX = OPERATOR | DELIMITER | END,
        ALL_TOKENS = 0xFF  // All bits
    };
	struct Token 
	{
		std::string value;
        TokenType type;
		size_t line, column;
	};
    class TokenMap {
    private:
        static const std::unordered_set<std::string>& GetIsOperatorSet() {
            static std::unordered_set<std::string> map = { "+", "-", "*", "/", ":", "::" };
            return map;
        }

        static const std::unordered_map<std::string, TokenType>& GetTokenTypeMap() {
            static std::unordered_map<std::string, TokenType> map = {
                {"+",   TokenType::OPERATOR},
                {"-",   TokenType::OPERATOR},
                {"*",   TokenType::OPERATOR},
                {"/",   TokenType::OPERATOR}, 
                {":",   TokenType::OPERATOR},
                {"::",  TokenType::OPERATOR},
                {",",   TokenType::DELIMITER},
                {"{",   TokenType::DELIMITER},
                {"}",   TokenType::DELIMITER},
                {"[",   TokenType::DELIMITER},
                {"]",   TokenType::DELIMITER},
                {";",   TokenType::END},        
                {"tag", TokenType::KEYWORD},
                {"copy", TokenType::KEYWORD},
                {"key", TokenType::KEYWORD},
            };
            return map;
        }

    public:
        static bool IsOperator(const std::string& str) {
            return GetIsOperatorSet().count(str) > 0;
        }

        static TokenType GetTType(const std::string& str) {
            auto& map = GetTokenTypeMap();
            auto it = map.find(str);
            return it != map.end() ? it->second : TokenType::UDE;
        }

        // Сделаем static
        static std::string GetOperator(const std::string& code, size_t i) {
            if (i >= code.size()) return "";

            std::string single_char(1, code[i]);
            if (!IsOperator(single_char)) return "";

            // Проверяем двухсимвольный оператор
            if (i + 1 < code.size()) {
                std::string two_chars = single_char + code[i + 1];
                if (IsOperator(two_chars)) return two_chars;
            }

            return single_char;
        }

        static bool IsNumber(const std::string& str) {
            // Простая проверка (можно улучшить)
            if (str.empty()) return false;
            size_t start = (str[0] == '-') ? 1 : 0;
            if (start >= str.size()) return false;

            bool has_dot = false;
            for (size_t i = start; i < str.size(); i++) {
                if (str[i] == '.') {
                    if (has_dot) return false; // Две точки
                    has_dot = true;
                }
                else if (!std::isdigit(str[i])) {
                    return false;
                }
            }
            return true;
        }

        static TokenType DetermineTokenType(const std::string& value) {
            // Сначала операторы и разделители
            if (TokenType t = GetTType(value); t != TokenType::UDE)
                return t;

            // Затем литералы
            if (IsNumber(value)) return TokenType::NUMBER_LITERAL;
            if (value == "true" || value == "false") return TokenType::BOOL_LITERAL;

            // Всё остальное - идентификатор
            return TokenType::IDENTIFIER;
        }
    };
}