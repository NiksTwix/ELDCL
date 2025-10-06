#pragma once
#include "..\Decoding\ContainersTree.hpp"



namespace DCL 
{
    class Serializator
    {
    public:
        static Serializator& Get()
        {
            static Serializator serializator;
            return serializator;
        }

        std::string Serialize(std::shared_ptr<ContainersTree> tree)
        {
            std::stringstream ss;
            SerializeContainer(ss, tree->GetGlobalFields(), 0);
            return ss.str();
        }

    private:
        void SerializeContainer(std::stringstream& ss,
            const std::vector<Field>& fields,
            int indent_level)
        {
            std::string indent(indent_level * 4, ' '); // 4 пробела на уровень

            for (const auto& field : fields) {
                if (field.isKey && !field.isContainer) {
                    // key Name = value
                    ss << indent << "key " << field.name << " = " << FieldValueToString(field.value) << ";\n";
                }
                else if (field.isContainer) {
                    // Контейнер: tag::type Name { ... }
                    ss << indent << "tag::" << field.container->tag
                        << " " << field.name << "\n";
                    ss << indent << "{\n";

                    // Рекурсивно сериализуем поля контейнера
                    SerializeContainer(ss, field.container->ordered_fields, indent_level + 1);

                    ss << indent << "}\n\n";
                }
                else {
                    // Обычное поле: Name: Value;
                    ss << indent << field.name << ": " << FieldValueToString(field.value) << ";\n";
                }
            }
        }

        std::string FieldValueToString(const Value& value)
        {
            std::string result;
            switch (value.type) {
            case ValueType::STRING:
                return EscapeString(value.strVal);

            case ValueType::ARRAY: {
                std::string result = "[";
                for (size_t i = 0; i < value.arrayVal->size(); i++) {
                    if (i > 0) result += ", ";
                    result += FieldValueToString((*value.arrayVal)[i]);
                }
                result += "]";
                return result;
            }
            default:
                return value.ToString();
            }
        }

        std::string EscapeString(const std::string& str)
        {
            std::string result;
            result.reserve(str.length() + 10); // Забронируем немного места

            result += "\"";
            for (char c : str) {
                if (c == '"') {
                    result += "\\\"";
                }
                else if (c == '\\') {
                    result += "\\\\";
                }
                else if (c == '\n') {
                    result += "\\n";
                }
                else if (c == '\t') {
                    result += "\\t";
                }
                else {
                    result += c;
                }
            }
            result += "\"";
            return result;
        }
    };
}