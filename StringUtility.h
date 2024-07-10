#pragma once
#include <string>

//文字コードユーティリティ
namespace StringUtility
{
	//stringをwstringに変換する
	std::wstring ConvertString(const std::string& str);

	//wstringをstringに変換する
	std::string ConvertString(const std::wstring& str);
}
