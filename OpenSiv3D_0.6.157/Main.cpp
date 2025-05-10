# include <Siv3D.hpp> // Siv3D v0.6.15
#include"IntaractiveSiv3D/Tools/YScriptSystem.h"

void Main()
{
	ys::YScript scr;
	auto state = scr.createState();

	//関数の登録
	state->registerFunction(U"print", [](const String& str) { Print << str; });
	state->registerFunction(U"print", [](int num) { Print << num; });
	state->registerFunction(U"print", [](double num) { Print << num; });

	while (System::Update())
	{
		if (SimpleGUI::Button(U"実行", { Scene::Width() - 100,10 }))
		{	
			scr.compile({ U"script.txt" });
			auto summary = scr.run(U"script.txt", {});
			if (auto exception = summary.getException())Print << exception;
			Print << U"終了";
		}
	}
}
