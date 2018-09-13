#pragma once
#include <string>
#include "Core/Asserts.h"

class GenericApplication
{
public:
	int ExecuteHostScript(std::string Filename, std::string Args, bool ShowOutput = false) { /*ensureMsgf(false, "Generic Application Called")*/ };
	static void InitTiming() {};
	static double Seconds() { };
	static void Sleep(float Milliseconds) {  };
	CORE_API static int64_t GetFileTimeStamp(const std::string Path);
};

