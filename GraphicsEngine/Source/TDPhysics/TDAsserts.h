#pragma once
#if 1
#define DebugEnsure(condition) if(!condition){ __debugbreak();}
#define AssertDebugBreak() __debugbreak();
#else
#define DebugEnsure(condition);
#define AssertDebugBreak()
#endif