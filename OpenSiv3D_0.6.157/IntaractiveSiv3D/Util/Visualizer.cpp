#include"../stdafx.h"
#include"Visualizer.h"
#include<windows.h>

size_t ys::getStringLength(const String& str)
{
	size_t ret = 0;
	for (size_t i = 0; i < str.length(); i++)
	{
		++ret;
		if (IsDBCSLeadByte(str[i]))++ret;
	}
	return ret;
}
