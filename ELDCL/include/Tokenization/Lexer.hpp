#pragma once
#include "TokensInfo.hpp"
#include <iostream>


namespace DCL 
{
	class Lexer 
	{
	private:
		bool m_debug_mode = false;

		std::string undefined_token = std::string();
		int current_line = 0;
		int current_column = 0;
		bool was_comment = false;
		bool is_negative_value = false; // -10

		void ResetState() 
		{
			undefined_token = std::string();
			current_line = 0;
			current_column = 0;
			was_comment = false;
			is_negative_value = false; 
		}


		char32_t ProcessESC(const std::string& code, size_t& i) {
			
			if (i + 1 >= code.size()) return 0;

			char32_t next = code[i + 1];
			if (next == '"') return '"';
			if (next == '\'') return '\'';
			if (next == 'n') return '\n';
			if (next == 't') return '\t';
			if (next == '\\') return '\\';
			return next;
		}
		Token HandleStringLiteral(const std::string& code, size_t& i)
		{
			std::string result;
			for (; i < code.size(); i++)
			{
				char c = code[i];  
				if (c == '"') {   
					current_column++; 
					break;
				}
				if (c == '\r') continue;
				// Обработка перевода строки
				if (c == '\n') {
					current_line++;
					current_column = 0;
					result.push_back(c);
					continue;
				}
				// Обработка экранирования
				if (c == '\\') {
					if (i + 1 >= code.size()) break; // Защита от выхода за границы
					char esc = ProcessESC(code, i);
					if (esc != 0) {
						result.push_back(esc);
						i++; // Пропускаем следующий символ (он уже обработан)
						current_column += 2; // Учитываем оба символа: \ и x
						continue;
					}
				}
				result.push_back(c);
				current_column++;
			}
			Token t;
			t.value = result;
			t.column = current_column;
			t.line = current_line;
			t.type = TokenType::STRING_LITERAL;
			return t;
		}
		bool IsDelimiter(char c) {
			TokenType type = TokenMap::DetermineTokenType(std::string(1, c));
			return (type & (TokenType::DELIMITER | TokenType::END)) != 0;
		}

		Token CreateToken(const std::string& value) {
			Token t;
			t.value = value;
			t.column = current_column;
			t.line = current_line;
			t.type = TokenMap::DetermineTokenType(value); // Нужно реализовать
			return t;
		}

		void PrintTokens(const std::vector<Token>& tokens) 
		{
			for (auto t : tokens) 
			{
				std::cout << t.value << "|" << t.line << "|" << t.column << "|" << (int)t.type << "\n";
			}
		}

	public:
		Lexer() = default;
		//Forbid copying
		Lexer(const Lexer&) = delete;
		Lexer& operator=(const Lexer&) = delete;
		//Forbid moving
		Lexer(Lexer&) = delete;
		Lexer& operator=(Lexer&) = delete;
		static Lexer& Get() 
		{
			static Lexer lexer;
			return lexer;
		}
		void SetDebugMode(bool value) { m_debug_mode = value; }


		std::vector<Token> ToTokens(const std::string& code)
		{
			ResetState();
			std::vector<Token> tokens;

			for (size_t i = 0; i < code.size(); i++)
			{
				char c = code[i];  // char вместо char32_t для простоты
				if (c == '\r') continue;

				current_column++;

				// Обработка строковых литералов
				if (c == '\"' && undefined_token.empty()) {
					tokens.push_back(HandleStringLiteral(code, ++i)); // ++i чтобы пропустить открывающую "
					continue;
				}

				// Комментарии
				if (c == '/' && i + 1 < code.size() && code[i + 1] == '/') {
					// Пропускаем до конца строки
					while (i < code.size() && code[i] != '\n') i++;
					current_line++;
					current_column = 0;
					continue;
				}

				// Обработка разделителей
				if (IsDelimiter(c)) {
					if (!undefined_token.empty()) {
						tokens.push_back(CreateToken(undefined_token));
						undefined_token.clear();
					}
					tokens.push_back(CreateToken(std::string(1, c)));
					continue;
				}

				// Пробелы и переводы строк
				if (std::isspace(c)) {
					if (!undefined_token.empty()) {
						tokens.push_back(CreateToken(undefined_token));
						undefined_token.clear();
					}
					if (c == '\n') {
						current_line++;
						current_column = 0;
					}
					continue;
				}

				if (std::string oper = TokenMap::GetOperator(code, i); oper != "") 
				{
					if (!undefined_token.empty()) {
						tokens.push_back(CreateToken(undefined_token));
						undefined_token.clear();
					}
					tokens.push_back(CreateToken(oper));
					i++;
					continue;
				}
				undefined_token.push_back(c);
			}

			// Не забыть последний токен
			if (!undefined_token.empty()) {
				tokens.push_back(CreateToken(undefined_token));
			}


			if (m_debug_mode) 
			{
				PrintTokens(tokens);
			}

			return tokens;
		}
	};
}