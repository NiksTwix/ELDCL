#pragma once
#include "ContainersTree.hpp"


namespace DCL 
{
	struct TokensBlock
	{
		std::vector<Token> header;
		std::vector<TokensBlock> body;
	};


	class Decoder 
	{
        // "Индексы" как в БД - быстрый поиск по key-полям
        std::unordered_map<std::string, std::shared_ptr<Container>> key_index;

        bool m_debug_mode;
        TokensBlock BuildBlocks(std::vector<Token>& tokens, size_t start_index = 0) {
            TokensBlock current;
            std::vector<Token> current_line;

            for (size_t i = start_index; i < tokens.size(); i++) {
                Token& t = tokens[i];

                if (t.value == "{") {
                    int depth = 1;
                    std::vector<Token> body;
                    i++;

                    while (depth > 0 && i < tokens.size()) {
                        Token& inner_t = tokens[i];  // Другое имя переменной!

                        if (inner_t.value == "{") depth++;
                        if (inner_t.value == "}") depth--;

                        if (depth > 0) {
                            body.push_back(inner_t);
                        }
                        else {
                            // Добавляем закрывающую } как отдельный блок
                            //TokensBlock close_block;
                            //close_block.header = { inner_t };
                            //current.body.push_back(close_block);
                        }
                        i++;
                    }
                    i--;  // Корректируем позицию

                    // Рекурсивно разбираем тело блока
                    auto tb = BuildBlocks(body);
                    tb.header = current_line;  // Заголовок блока (до {)
                    current.body.push_back(tb);
                    current_line.clear();
                    continue;
                }

                if (t.value == ";") {
                    if (!current_line.empty()) {
                        TokensBlock tb;
                        tb.header = current_line;
                        current.body.push_back(tb);
                        current_line.clear();
                    }
                    continue;
                }

                current_line.push_back(t);
            }

            // Последняя строка без ;
            if (!current_line.empty()) {
                TokensBlock tb;
                tb.header = current_line;
                current.body.push_back(tb);
            }

            return current;
        }
        // === ФАЗА 1: ПОСТРОЕНИЕ СТРУКТУРЫ ===

        void BuildField(std::stack<std::shared_ptr<Container>>& containers_stack,
            std::vector<Token>& tokens)
        {
            if (containers_stack.empty() || tokens.empty()) return;
            auto current_context = containers_stack.top();

            // Специальные конструкции
            if (tokens[0].value == "copy" && tokens.size() > 1) {
                current_context->pending_copies.push_back(tokens[1].value);
                return;
            }

            if (tokens[0].value == "key") {
                std::string field_name = tokens[1].value;
                std::vector<Token> value_tokens(tokens.begin() + 3, tokens.end());
                auto f = Field(field_name, Value(), true);
                f.unresolved_tokens = value_tokens;

                current_context->ordered_fields.push_back(f);
                return;
            }

            // Обычные поля
            std::string field_name;
            std::vector<Token> value_tokens;
            if (!ParseFieldAssignment(tokens, field_name, value_tokens)) {
                return;
            }

            auto f = Field(field_name, Value(), false);
            f.unresolved_tokens = value_tokens;

            current_context->ordered_fields.push_back(f);

        }

        bool ParseFieldAssignment(const std::vector<Token>& tokens,
            std::string& field_name,
            std::vector<Token>& value_tokens)
        {
            for (size_t i = 0; i < tokens.size(); i++) {
                if (tokens[i].value == ":" && i > 0 && tokens[i - 1].type == TokenType::IDENTIFIER) {
                    field_name = tokens[i - 1].value;
                    for (size_t j = i + 1; j < tokens.size(); j++) {
                        if (tokens[j].type != TokenType::END) {
                            value_tokens.push_back(tokens[j]);
                        }
                    }
                    return true;
                }
            }
            return false;
        }

        std::shared_ptr<Container> BuildTree(TokensBlock block)
        {
            auto root_container = std::make_shared<Container>();
            root_container->name = "root";

            std::stack<std::shared_ptr<Container>> containers_stack;
            containers_stack.push(root_container);

            std::function<void(TokensBlock&, std::shared_ptr<Container>)> processBlock;

            processBlock = [&](TokensBlock& current_block, std::shared_ptr<Container> current_container) {
                for (auto& body_block : current_block.body) {
                    if (IsContainerBlock(body_block)) {
                        auto child_container = std::make_shared<Container>();
                        child_container->parent = current_container;
                        ParseContainerHeader(body_block.header, *child_container);
                        auto f = Field(child_container->name, child_container);
                        f.isContainer = true;
                        current_container->ordered_fields.push_back(f);
                        containers_stack.push(child_container);
                        processBlock(body_block, child_container);
                        containers_stack.pop();
                    }
                    else {
                        BuildField(containers_stack, body_block.header);
                    }
                }
                };

            processBlock(block, root_container);
            return root_container;
        }

        // === ФАЗА 2: ПОСТОБРАБОТКА ===

        void ResolveAllReferences(std::shared_ptr<Container> container)
        {
            std::string new_name = container->name; // Изначальное имя

            // 1. Разрешаем ссылки и находим key-поле
            for (auto& field : container->ordered_fields) {
                if (!field.unresolved_tokens.empty()) {
                    field.value = ParseValue(field.unresolved_tokens, container);
                    field.unresolved_tokens.clear();

                    // Запоминаем новое имя из key-поля
                    if (field.isKey && !field.isContainer &&
                        (field.value.type == ValueType::NUMBER || field.value.type == ValueType::STRING))
                    {
                        container->key = &field;
                        key_index[field.value.ToString()] = container;
                    }
                }
            }
            for (const auto& source_name : container->pending_copies) {
                ProcessCopy(container, source_name);
            }
            container->pending_copies.clear();

            // Рекурсия
            for (auto& field : container->ordered_fields) {
                if (field.isContainer && field.container) {
                    ResolveAllReferences(field.container);
                }
            }
        }

        Value ParseValue(const std::vector<Token>& tokens, std::shared_ptr<Container> current_context)
        {
            if (tokens.empty()) return Value();

            // Массивы
            if (tokens[0].value == "[") {
                auto array_val = std::make_shared<std::vector<Value>>();
                for (size_t i = 1; i < tokens.size(); i++) {
                    if (tokens[i].value == "]") break;
                    if (tokens[i].value == ",") continue;
                    std::vector<Token> element_tokens = { tokens[i] };
                    array_val->push_back(ParseValue(element_tokens, current_context));
                }
                return Value(array_val);
            }

            // Одиночные значения
            if (tokens.size() == 1) {
                const Token& t = tokens[0];
                if (t.type == TokenType::NUMBER_LITERAL) return Value(std::stod(t.value));
                if (t.type == TokenType::STRING_LITERAL) return Value(t.value);
                if (t.type == TokenType::BOOL_LITERAL) return Value(t.value == "true");
                if (t.type == TokenType::IDENTIFIER) {
                    return FindFieldInContainers(t.value, current_context);
                }
            }

            // Scoped ссылки
            if (tokens.size() >= 3 && tokens[1].value == "::") {
                std::string container_name = tokens[0].value;
                std::string field_name = tokens[2].value;
                auto container = FindContainer(container_name, current_context);
                if (container) {
                    for (auto& f : container->ordered_fields) 
                    {
                        if (f.name == field_name && !f.isContainer) 
                        {
                            return f.value;
                        }
                    }
                }
            }

            return Value();
        }

        void ProcessCopy(std::shared_ptr<Container> target, const std::string& source_name)
        {
            auto source = FindContainer(source_name, target);
            if (!source) return;

            for (auto& field : source->ordered_fields) {
                if (!field.isContainer) {
                    // Пропускаем, если поле УЖЕ существует в цели. Наследование добавляет новые поля, явно заданные оно не меняет!
                    bool skip = false;
                    for (auto& f : target->ordered_fields)
                    {
                        if (f.name == field.name)
                        {
                            skip = true;
                            break;
                        }
                    }
                    if (skip) continue;

                    if (field.unresolved_tokens.size() > 0) {
                        field.value = ParseValue(field.unresolved_tokens, source);
                        field.unresolved_tokens.clear();
                    }

                    target->ordered_fields.push_back(field);
                }
            }
        }

        // === ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ ===

        Value FindFieldInContainers(const std::string& field_name, std::shared_ptr<Container> start)
        {
            auto current = start;
            while (current != nullptr) {
                for (auto& f : current->ordered_fields)
                {
                    if (f.name == field_name)
                    {
                        return f.value;
                    }
                }
                current = current->parent;
            }
            return Value();
        }

        std::shared_ptr<Container> FindContainer(const std::string& name, std::shared_ptr<Container> start)
        {
            auto current = start;
            while (current != nullptr) {
                for (const auto& field : current->ordered_fields) {
                    if (field.isContainer && field.container->name == name) {
                        return field.container;
                    }
                }
                current = current->parent;
            }
            return nullptr;
        }

        bool IsContainerBlock(const TokensBlock& block) {
            if (block.body.size() > 0) return true;
            for (size_t i = 0; i < block.header.size(); i++) {
                if (block.header[i].value == "tag" && i + 1 < block.header.size() &&
                    block.header[i + 1].value == "::") {
                    return true;
                }
            }
            return false;
        }

        void ParseContainerHeader(const std::vector<Token>& header, Container& container) {
            for (size_t i = 0; i < header.size(); i++) {
                if (header[i].value == "tag" && i + 2 < header.size() &&
                    header[i + 1].value == "::") {
                    container.tag = header[i + 2].value;
                    i += 2;
                }
                else if (header[i].type == TokenType::IDENTIFIER && container.name == "container0") {
                    container.name = header[i].value;
                }
            }
        }
    public:
        void SetDebugMode(bool value) { m_debug_mode = value; }
        static Decoder& Get()
        {
            static Decoder decoder;
            return decoder;
        }
        std::shared_ptr<ContainersTree> Decode(std::vector<Token>& tokens)
        {
            key_index.clear();
            // Фаза 1: Построение структуры
            TokensBlock tb = BuildBlocks(tokens);
            auto root_container = BuildTree(tb);

            // Фаза 2: Постобработка
            ResolveAllReferences(root_container);

            auto CT = std::make_shared<ContainersTree>(root_container->ordered_fields, key_index);
            return CT;
        }
    };

}


