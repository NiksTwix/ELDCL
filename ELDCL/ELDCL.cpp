// ELDCL.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include "include/Tokenization/Lexer.hpp"
#include "include/Loading\Loader.hpp"
#include "include/Serialization\Serializator.hpp"
//ELDCL - EndLoad Data Containing Language

int main()
{
    std::string code = R"L(
tag::constants SContainer
{
	Key: 200;	//Field
	Key1: 20023;	//Field
	Keys: ["list","234",true,false];
	String: "
		For EndLoad Engine v0.2
	";
	ConstantNumber: Key;
	ConstantNumber1: Key1;
}
tag::container Container1 	//Tag "container" sets explicitly
{
	ConstantKey: SContainer::Key;
	ConstantNumber: SContainer::ConstantNumber1; 	//Handle by alu stack machine
}
KEY1: 10;
tag::entity Entity10
{
	copy SContainer;
	copy Container1;	
	ConstantNumber: 200234000;
	key Name = "123243";	
	ID: 10;
	PARENT_ID: 2;
	tag::component Transform
	{
		Origin: [10.0,20.0,3.04];
		ID: 2324; 
		
	}
	tag::component Transform1
		{
			copy Transform;
			Origin1: Transform::Origin;
			
		}	
}
	)L";
	auto ct = DCL::Loader::LoadFromString(code);
	auto t = DCL::Serializator::Get().Serialize(ct);
	std::cout << t << "\n";
	auto ct1 = DCL::Loader::LoadFromString(t);
	ct1->PrintFields();

	//DCL::ContainersTree ct;
	//ct.GetField("key::234::234::234::234");
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
