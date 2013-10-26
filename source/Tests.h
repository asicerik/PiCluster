#pragma once

class Tests
{
public:
	Tests();
	~Tests();
	bool RunAllTests();
	bool RunMemoryTests();
	bool RunGeometryTests();
	bool RunFramebufferTests();
private:
	bool PassRectByVal(Rect rect);
	bool PassRectByRef(Rect& rect);
	bool PassRectByConstRef(const Rect& rect);
	bool PassRectByPtr(Rect* rect);
	Rect  GetRectByVal();
	Rect& GetRectByRef();
	Rect* GetRectByPtr();
	Rect	mPrivateRect;
};
