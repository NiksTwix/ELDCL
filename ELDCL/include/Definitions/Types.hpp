#pragma once
#include <vector>
#include <memory>
#include <string>
#include <queue>
#include <stack>
#include <unordered_map>
#include <functional>
#include <unordered_set>

namespace DCL 
{
    enum ValueType { VOID, NUMBER, STRING, BOOL, ARRAY, CONTAINER }; 

    struct Value;
    struct Field;
    

    struct Container 
    {
        std::string tag = "container";
        std::string name = "container0";
        std::vector<Field> ordered_fields;
        std::shared_ptr<Container> parent;
        std::vector<std::string> pending_copies;

        Field* key;

        inline Field* FindField(const std::string& name);

        inline std::unordered_map<std::string, Field*> FindFields(const std::vector<std::string>& field_names);

    };

    struct Value {          //Took from ELScript
        ValueType type;

        union {
            double numberVal;
            bool boolVal;
        };
        std::string strVal;   // For strings
        std::shared_ptr<std::vector<Value>> arrayVal; // For arrays
        // ������������
        Value() : type(ValueType::VOID), numberVal(0) {}

        Value(double v) : type(ValueType::NUMBER), numberVal(v) {}
        Value(int v) : type(ValueType::NUMBER), numberVal(v) {}

        Value(bool v) : type(ValueType::BOOL), boolVal(v) {}

        Value(const std::string& s) : type(ValueType::STRING) {
            new (&strVal) std::string(s);
        }

        Value(const char* s) : type(ValueType::STRING) {
            new (&strVal) std::string(s);
        }

        Value(std::shared_ptr<std::vector<Value>> arr) : type(ValueType::ARRAY), arrayVal(arr) {}
        // ������� ���� (����� �����!)
        // ����������� �����������
        Value(const Value& other) : type(other.type) {
            switch (type) {
            case ValueType::NUMBER: numberVal = other.numberVal; break;
            case ValueType::BOOL:   boolVal = other.boolVal; break;
            case ValueType::STRING: new (&strVal) std::string(other.strVal); break; // �������� ������
            case ValueType::ARRAY:  arrayVal = std::make_shared<std::vector<Value>>(*other.arrayVal); break; // �������� ������ ���������
            case ValueType::VOID:   break; // ������ �� ������
            }
        }

        // �������� ������������
        Value& operator=(const Value& other) {
            if (this == &other) return *this;
            destroyCurrent();

            type = other.type;
            switch (type) {
            case ValueType::NUMBER: numberVal = other.numberVal; break;
            case ValueType::BOOL:   boolVal = other.boolVal; break;
            case ValueType::STRING: new (&strVal) std::string(other.strVal); break;
            case ValueType::ARRAY:  arrayVal = std::make_shared<std::vector<Value>>(*other.arrayVal); break;
            case ValueType::VOID:   break;
            }
            return *this;
        }
        Value(Value&& other) noexcept : type(other.type) {
            switch (type) {
            case ValueType::NUMBER: numberVal = other.numberVal; break;
            case ValueType::BOOL:   boolVal = other.boolVal; break;
            case ValueType::STRING: new (&strVal) std::string(std::move(other.strVal)); break;
            case ValueType::ARRAY:  arrayVal = std::move(other.arrayVal); break; // �����: ���������� shared_ptr
            case ValueType::VOID:   break;
            }
            // ����� ����������� ����� �������� �������� ������ � ���������� ���������!
            other.type = ValueType::VOID; // ����� ���������� other ������ �� �����
        }

        // �������� ������������� ������������
        Value& operator=(Value&& other) noexcept {
            if (this != &other) {
                destroyCurrent(); // ���������� ������� ������
                type = other.type;
                switch (type) {
                case ValueType::NUMBER: numberVal = other.numberVal; break;
                case ValueType::BOOL:   boolVal = other.boolVal; break;
                case ValueType::STRING: new (&strVal) std::string(std::move(other.strVal)); break;
                case ValueType::ARRAY:  arrayVal = std::move(other.arrayVal); break;
                }
                other.type = ValueType::VOID;
            }
            return *this;
        }
        static std::string GetTypeString(ValueType type)
        {
            std::string result;
            switch (type)
            {
            case ValueType::VOID:
                return "void";
            case ValueType::NUMBER:
                return "number";
            case ValueType::STRING:
                return "string";
            case ValueType::BOOL:
                return "bool";
            case ValueType::ARRAY:
                return "array";
            default:
                return "void";
            }
        }

        std::string ToString() const
        {
            std::string result;
            switch (type)
            {
            case VOID:
                return "";
            case NUMBER:
                if (EqualAround(numberVal, (int)numberVal), 0.0001f) return std::to_string((int)numberVal);
                return std::to_string(numberVal);
            case STRING:
                return strVal;
            case BOOL:
                return boolVal ? "true" : "false";
            case ARRAY:
                for (auto l : *arrayVal)
                {
                    
                    result += l.ToString() + ",";
                    
                }
                if (result.back() == ',')result.pop_back();
                return result;

            default:
                return "";
            }
        }


        // ����������
        ~Value() {
            destroyCurrent();
        }
    private:
        void destroyCurrent() {
            // ���� �������� ���������� ������ ��� ������
            if (type == ValueType::STRING) {
                strVal.~basic_string();
            }
            // ��� shared_ptr � ���������� ���������� �������� �� �����
        }

        bool EqualAround(float n1, float n2, float interval = 0.01f) const  
        {
            return n2 <= n1 + interval && n2 >= n1 - interval;
        }

    };
    
    struct Token;

    struct Field
    {
        bool isKey;
        bool isContainer;
        std::string name;
        Value value;
        std::shared_ptr<Container> container;
        std::vector<Token> unresolved_tokens;
        // ������������
        Field(std::string name, std::shared_ptr<Container> con, bool is_key = false)
            : name(std::move(name)), container(std::move(con)), isContainer(true), isKey(is_key) {
        }

        Field(std::string name, Value val, bool is_key = false)
            : name(std::move(name)), value(std::move(val)), isContainer(false), isKey(is_key) {
        }

        Field() : isContainer(false), isKey(false) {}

        // 1. ���������� (�� ����� �����, �.�. ��� ����� ���� ��������� ���������)
        ~Field() = default;

        // 2. ����������� �����������
        Field(const Field& other)
            : isKey(other.isKey),
            isContainer(other.isContainer),
            name(other.name),
            value(other.value),  // Value ������ ����� ����������� �����������
            container(other.container),  // shared_ptr ���������� � ����������� ��������
            unresolved_tokens(other.unresolved_tokens)
        {
        }

        // 3. �������� ������������ ������������
        Field& operator=(const Field& other) {
            if (this != &other) {
                isKey = other.isKey;
                // isContainer - �����������, �� ����� ��������
                name = other.name;
                value = other.value;
                container = other.container;
                unresolved_tokens = other.unresolved_tokens;
            }
            return *this;
        }

        // 4. ����������� �����������
        Field(Field&& other) noexcept
            : isKey(other.isKey),
            isContainer(other.isContainer),
            name(std::move(other.name)),
            value(std::move(other.value)),
            container(std::move(other.container)),
            unresolved_tokens(std::move(other.unresolved_tokens))
        {
            // �������� other � �������� ���������
            other.isKey = false;
            // other.isContainer ������� ��� ���� (���������)
        }

        // 5. �������� ������������ ������������
        Field& operator=(Field&& other) noexcept {
            if (this != &other) {
                isKey = other.isKey;
                // isContainer - �����������, �� ����� ��������
                name = std::move(other.name);
                value = std::move(other.value);
                container = std::move(other.container);
                unresolved_tokens = std::move(other.unresolved_tokens);
                other.isKey = false;
            }
            return *this;
        }
    };

    inline Field* Container::FindField(const std::string& name)
    {
        for (auto& field : ordered_fields) {
            if (field.name == name) return &field;
        }
        return nullptr;
    }
    inline std::unordered_map<std::string, Field*> Container::FindFields(const std::vector<std::string>& field_names)
    {
        std::unordered_map<std::string, Field*> result;

        // ������������� �����������
        for (const auto& name : field_names) {
            for (auto& field : ordered_fields) {
                if (field.name == name) {
                    result[name] = &field;
                    break;
                }
            }
            // ���� �� ����� - �� ��������� � ��������� (��� ����� �������� nullptr)
        }

        return result;
    }
}