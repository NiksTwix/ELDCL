#pragma once
#include "..\Tokenization\TokensInfo.hpp"
#include "..\Definitions\StringOperations.hpp"

namespace DCL 
{
	class ContainersTree 
	{
		std::vector<Field> global_fields;

        // "Индексы" как в БД - быстрый поиск по key-полям
        std::unordered_map<std::string, std::shared_ptr<Container>> key_index;

	public:
        ContainersTree(std::vector<Field>& fields) : global_fields(fields) {}
        ContainersTree(std::vector<Field>& fields, std::unordered_map<std::string, std::shared_ptr<Container>>& key_index) : global_fields(fields), key_index(key_index) {}

		//KEY::A::B
        // Поиск поля по абсолютному пути "Container::Field"
        Value GetField(const std::string& absolute_path) const {
            auto path_parts = StringOperations::SplitString(absolute_path, "::");
            if (path_parts.empty()) return Value();

            // Ищем стартовый контейнер в глобальных полях
            const Field* current_field = nullptr;
            for (const auto& field : global_fields) {
                if (field.name == path_parts[0] && field.isContainer) {
                    current_field = &field;
                    break;
                }
            }

            if (!current_field || !current_field->container) {
                return Value(); // Контейнер не найден
            }

            // Идём по цепочке контейнеров
            auto current_container = current_field->container;
            for (size_t i = 1; i < path_parts.size(); i++) {
                if (i == path_parts.size() - 1) {
                    // Последняя часть - ищем поле
                    for (const auto& field : current_container->ordered_fields) {
                        if (field.name == path_parts[i] && !field.isContainer) {
                            return field.value;
                        }
                    }
                }
                else {
                    // Промежуточная часть - ищем контейнер
                    bool found = false;
                    for (const auto& field : current_container->ordered_fields) {
                        if (field.name == path_parts[i] && field.isContainer && field.container) {
                            current_container = field.container;
                            found = true;
                            break;
                        }
                    }
                    if (!found) return Value();
                }
            }
            return Value();
        }


        std::vector<std::shared_ptr<Container>> GetByTag(const std::string& tag_value, std::shared_ptr<Container> where = nullptr, bool deep = false)const
        {
            std::vector<std::shared_ptr<Container>> containers;
            std::queue<std::shared_ptr<Container>> deep_containers;
            std::unordered_set<std::shared_ptr<Container>> visited; // Защита от циклов

            const std::vector<Field>& fields = (where != nullptr) ? where->ordered_fields : global_fields;

            // Начальные контейнеры
            for (auto& f : fields) {
                if (f.isContainer && f.container && f.container->tag == tag_value) {
                    if (visited.insert(f.container).second) { // Проверяем, не посещали ли уже
                        deep_containers.push(f.container);
                    }
                }
            }

            // BFS обход
            while (!deep_containers.empty()) {
                auto container = deep_containers.front();
                deep_containers.pop();

                containers.push_back(container);

                if (deep) {
                    for (auto& f : container->ordered_fields) {
                        if (f.isContainer && f.container && f.container->tag == tag_value) {
                            if (visited.insert(f.container).second) { // Избегаем повторов
                                deep_containers.push(f.container);
                            }
                        }
                    }
                }
            }

            return containers;
        }


        // Быстрый поиск по key (аналог SELECT * FROM containers WHERE key = 'player_1')
        std::shared_ptr<Container> FindByKey(const std::string& key_value) const {
            auto it = key_index.find(key_value);
            return it != key_index.end() ? it->second : nullptr;
        }

		void PrintField(Field& field) 
		{
			if (field.isContainer) 
			{
				std::cout << "Container (" << field.container->tag + "): " << field.name << "{\n";

				for (auto& f : field.container->ordered_fields)
				{
					PrintField(f);
				}
				std::cout << "}\n";
				return;
			}

			std::cout << "Field: " << field.name << " |" << (field.isKey ? "key" : "not a key") << "| " << field.value.ToString() << "\n";
		}

		void PrintFields() {

			for (auto& f : global_fields) 
			{
				PrintField(f);
			}
		}
	};
}