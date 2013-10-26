#include "stddef.h"
#include "stdarg.h"
#include "stdint.h"
#include "stdlib.h"
#include <math.h>
#include "uart0.h"
#ifdef WIN32
#include "windows.h"
#include "winsock.h"
#include <string>
#endif
#include "GraphicsShim.h"
#include "Region.h"
#include "Tests.h"

#ifdef WIN32 
#define NO_OPT
#else
#define NO_OPT __attribute__((optimize("O0"))) 
#endif

Tests::Tests()
{
}

Tests::~Tests()
{
}

bool
Tests::RunAllTests()
{
	bool res = true;
	do
	{
//		res = RunMemoryTests();
//		if (!res)	break;

		res = RunGeometryTests();
		if (!res)	break;

		res = RunFramebufferTests();
		if (!res)	break;

	} while (false);
	return res;
}

bool NO_OPT
Tests::RunMemoryTests()
{
	bool res = true;
	uint32_t* mem = NULL;
	size_t memSizeMB = 16;	// How many megabytes of RAM to test. This number should be bigger than
							// your platform's available RAM to make sure memory is being freed properly.
	do
	{
		UartPrintf("RunMemoryTests : running...\n");
		UartPrintf("RunMemoryTests : alloc/free tests...\n");
		// Allocate 1MB memory chunks memSizeMB times to make sure the system is freeing the memory properly
		for (size_t i=0;i<memSizeMB;i++)
		{
			UartPrintf("\rRunMemoryTests : %d/%d            ", i, memSizeMB);
			// Test malloc()
			mem = (uint32_t*)malloc(1024*1024);
			if (!mem)
			{
				UartPrintf("malloc() test failed\n");
				res = false;
				break;
			}
			// Fill the memory to make sure we can actually read and write it
			for (uint32_t val=0;val<256*1024;val++)
			{
				mem[val] = val + i;
			}
			
			// Read it back
			for (uint32_t val=0;val<256*1024;val++)
			{
				if (mem[val] != (val + i))
				{
					UartPrintf("malloc() test failed read write test\n");
					res = false;
					break;
				}
			}
			free(mem);

			// Test new()
			try
			{
				mem = new uint32_t[256*1024];
			}
			catch (...)
			{
				UartPrintf("new() test failed\n");
				res = false;
				break;
			}
			// Fill the memory to make sure we can actually read and write it
			for (uint32_t val=0;val<256*1024;val++)
			{
				mem[val] = val + i;
			}
			
			// Read it back
			for (uint32_t val=0;val<256*1024;val++)
			{
				if (mem[val] != (val + i))
				{
					UartPrintf("malloc() test failed read write test\n");
					res = false;
					break;
				}
			}
			delete [] mem;
		}

		UartPrintf("\nRunMemoryTests : 2GB alloc tests...\n");
		mem = (uint32_t*)malloc(0x7fffffff);	// 2GB
		if (mem)
		{
			UartPrintf("malloc() 2GB test failed\n");
			res = false;
			break;
		}

		// Test new()
		try
		{
			mem = new uint32_t[0x1fffffff];
			UartPrintf("new() 2GB test failed\n");
			res = false;
			break;
		}
		catch (...)
		{
		}

		UartPrintf("RunMemoryTests : Stack alloc tests...\n");
		// Allocate 4kB memory chunks enough times to get memSizeMB total to make sure the system is freeing the memory properly
		// 4kB * 128 * 2 (16 bit vals are 2 bytes each) = 2^12 + 2^7 + 2^1 = 2^20 = 1MB
		for (uint16_t i=0;((i<memSizeMB*128) && (res==true));i++)
		{
			if ((i % 128) == 0)
				UartPrintf("\rRunMemoryTests : stack %d/%d            ", i/128, memSizeMB);

			// local variable on the stack
			uint16_t local[4096];

			// Fill the memory to make sure we can actually read and write it
			for (uint16_t val=0;val<4096;val++)
			{
				local[val] = val + i;
			}
			
			// Read it back
			for (uint16_t val=0;val<4096;val++)
			{
				if (local[val] != (val + i))
				{
					UartPrintf("Stack alloc test failed read write test\n");
					res = false;
					break;
				}
			}
		}

	} while (false);

	if (res)
		UartPrintf("\nRunMemoryTests : passed\n");
	else
		UartPrintf("\nRunMemoryTests : failed\n");

	return res;
}

bool
Tests::RunGeometryTests()
{
	bool res = false;
	do
	{
		UartPrintf("RunGeometryTests : running...\n");

		UartPrintf("RunGeometryTests : Point tests...\n");
		Point point(1000, -1000);
		if (point.x != 1000 || point.y != -1000)
		{
			UartPrintf("Point CTOR initialization fail\n");
			break;
		}

		Point point2(point);
		if (point2.x != 1000 || point2.y != -1000)
		{
			UartPrintf("Point CTOR ref initialization fail\n");
			break;
		}

		Point point3 = point;
		if (point3.x != 1000 || point3.y != -1000)
		{
			UartPrintf("Point assignment fail\n");
			break;
		}

		UartPrintf("RunGeometryTests : Rectangle tests...\n");
		Rect rect(1000, -1000, -8000, 8000);
		if (rect.x != 1000 || rect.y != -1000 || rect.w != -8000 || rect.h != 8000)
		{
			UartPrintf("Rect CTOR initialization fail\n");
			break;
		}

		Rect rect2(rect);
		if (rect2.x != 1000 || rect2.y != -1000 || rect2.w != -8000 || rect2.h != 8000)
		{
			UartPrintf("Rect CTOR ref initialization fail\n");
			break;
		}

		mPrivateRect = rect;
		if (mPrivateRect.x != 1000 || mPrivateRect.y != -1000 || mPrivateRect.w != -8000 || mPrivateRect.h != 8000)
		{
			UartPrintf("Rect stack assignment fail\n");
			break;
		}

		Rect* rect4 = new Rect();
		*rect4 = rect;
		if (rect4->x != 1000 || rect4->y != -1000 || rect4->w != -8000 || rect4->h != 8000)
		{
			UartPrintf("Rect heap assignment fail\n");
			break;
		}
		delete rect4;

		if (!PassRectByVal(rect))
		{
			UartPrintf("PassRectByVal fail\n");
			break;
		}

		if (!PassRectByRef(rect))
		{
			UartPrintf("PassRectByRef fail\n");
			break;
		}

		const Rect constRect = rect;
		if (!PassRectByConstRef(constRect))
		{
			UartPrintf("PassRectByRef fail\n");
			break;
		}

		if (!PassRectByPtr(&rect))
		{
			UartPrintf("PassRectByPtr fail\n");
			break;
		}

		rect = GetRectByVal();
		if (rect.x != mPrivateRect.x || rect.y != mPrivateRect.y || rect.w != mPrivateRect.w || rect.h != mPrivateRect.h)
		{
			UartPrintf("GetRectByVal fail\n");
			break;
		}

		rect = GetRectByRef();
		if (rect.x != mPrivateRect.x || rect.y != mPrivateRect.y || rect.w != mPrivateRect.w || rect.h != mPrivateRect.h)
		{
			UartPrintf("GetRectByRef fail\n");
			break;
		}

		rect4 = GetRectByPtr();
		if (rect4->x != mPrivateRect.x || rect4->y != mPrivateRect.y || rect4->w != mPrivateRect.w || rect4->h != mPrivateRect.h)
		{
			UartPrintf("GetRectByPtr fail\n");
			break;
		}

		UartPrintf("RunGeometryTests : Region tests...\n");
		Region region;
		if (!region.Empty())
		{
			UartPrintf("Region was not empty after CTOR\n");
			break;
		}

		rect = region.GetDirtyRect();
		if (rect.x != 0 || rect.y != 0 || rect.w != 0 || rect.h != 0)
		{
			UartPrintf("Region DirtyRect initialization fail\n");
			break;
		}

		if (!region.GetDirtyRects().empty())
		{
			UartPrintf("Region GetDirtyRects() was not empty after CTOR\n");
			break;
		}

		rect2 = Rect(-100, -200, 300, 400);
		region.AddRect(rect2);
		rect = region.GetDirtyRect();
		if (rect.x != rect2.x || rect.y != rect2.y || rect.w != rect2.w || rect.h != rect2.h)
		{
			UartPrintf("Region AddRect fail\n");
			break;
		}

		if (region.GetDirtyRects().size() != 1 ||
				region.GetDirtyRects()[0].x != rect2.x || region.GetDirtyRects()[0].y != rect2.y ||
				region.GetDirtyRects()[0].w != rect2.w || region.GetDirtyRects()[0].h != rect2.h)
		{
			UartPrintf("Region AddRect/GetDirtyRects fail\n");
			break;
		}

		UartPrintf("RunGeometryTests : Color32 tests...\n");
		Color32 color(1,2,3,4);
		if (color.r != 1 || color.g != 2 || color.b != 3 || color.a != 4)
		{
			UartPrintf("Color CTOR initialization fail\n");
			break;
		}

		Color32 color2(color);
		if (color2.r != 1 || color2.g != 2 || color2.b != 3 || color2.a != 4)
		{
			UartPrintf("Color CTOR ref initialization fail\n");
			break;
		}

		Color32 color3 = color;
		if (color3.r != 1 || color3.g != 2 || color3.b != 3 || color3.a != 4)
		{
			UartPrintf("Color assignment fail\n");
			break;
		}

		Color32 color4(0,0,0,eOpaque);
		Color32 color5(32, 64, 128, 128);
		color4 = color5.AlphaBlend(color4);
		if (color4.r != 16 || color4.g != 32 || color4.b != 64)
		{
			UartPrintf("Color AlphaBlend to black fail\n");
			break;
		}

		color4 = Color32(255,255,255,eOpaque);
		color5 = Color32(32, 64, 128, 128);
		color4 = color5.AlphaBlend(color4);
		if (color4.r != 142 || color4.g != 158 || color4.b != 190)
		{
			UartPrintf("Color AlphaBlend to white fail\n");
			break;
		}

		res = true;

	} while (false);

	if (res)
		UartPrintf("RunGeometryTests : passed\n");
	else
		UartPrintf("RunGeometryTests : failed\n");
	
	return res;
}

bool Tests::PassRectByVal(Rect rect)
{
	UartPrintf("PassRectByVal : rect address = %p\n", &rect);
	UartPrintf("PassRectByVal : rect = %d,%d - %d,%d\n",
			rect.x,
			rect.y,
			rect.w,
			rect.h);

	if (rect.x != mPrivateRect.x || rect.y != mPrivateRect.y || rect.w != mPrivateRect.w || rect.h != mPrivateRect.h)
		return false;
	
	Rect local = rect;
	if (rect.x != local.x || rect.y != local.y || rect.w != local.w || rect.h != local.h)
		return false;
	
	return true;
}

bool Tests::PassRectByRef(Rect& rect)
{
	UartPrintf("PassRectByRef : rect address = %p\n", &rect);
	UartPrintf("PassRectByRef : rect = %d,%d - %d,%d\n",
			rect.x,
			rect.y,
			rect.w,
			rect.h);

	if (rect.x != mPrivateRect.x || rect.y != mPrivateRect.y || rect.w != mPrivateRect.w || rect.h != mPrivateRect.h)
		return false;
	
	Rect local = rect;
	if (rect.x != local.x || rect.y != local.y || rect.w != local.w || rect.h != local.h)
		return false;
	
	return true;
}

bool Tests::PassRectByConstRef(const Rect& rect)
{
	UartPrintf("PassRectByConstRef : rect address = %p\n", &rect);
	UartPrintf("PassRectByConstRef : rect = %d,%d - %d,%d\n",
			rect.x,
			rect.y,
			rect.w,
			rect.h);

	if (rect.x != mPrivateRect.x || rect.y != mPrivateRect.y || rect.w != mPrivateRect.w || rect.h != mPrivateRect.h)
		return false;
	
	Rect local = rect;
	if (rect.x != local.x || rect.y != local.y || rect.w != local.w || rect.h != local.h)
		return false;
	
	return true;
}

bool Tests::PassRectByPtr(Rect* rect)
{
	UartPrintf("PassRectByPtr : rect address = %p\n", rect);
	UartPrintf("PassRectByPtr : rect = %d,%d - %d,%d\n",
			rect->x,
			rect->y,
			rect->w,
			rect->h);

	if (rect->x != mPrivateRect.x || rect->y != mPrivateRect.y || rect->w != mPrivateRect.w || rect->h != mPrivateRect.h)
		return false;
	
	Rect local = *rect;
	if (rect->x != local.x || rect->y != local.y || rect->w != local.w || rect->h != local.h)
		return false;
	
	return true;
}

Rect Tests::GetRectByVal()
{
	return mPrivateRect;
}

Rect& Tests::GetRectByRef()
{
	return mPrivateRect;
}

Rect* Tests::GetRectByPtr()
{
	return &mPrivateRect;
}

bool
Tests::RunFramebufferTests()
{
	bool res = false;
	do
	{
		UartPrintf("RunFramebufferTests : running...\n");

		FramebufferProperties priProps;

		// First, allocate our primary frame buffer so we have somewhere to draw
		GraphicsContext primary;

		// Something that wont fill the whole screen
		priProps.mBitsPerPixel	= 32;
		priProps.mBorderSize	= 0;
		priProps.mDoubleBuffer	= false;
		priProps.mGeometry.x	= 0;
		priProps.mGeometry.y	= 0;
		priProps.mGeometry.w	= 320;
		priProps.mGeometry.h	= 320;

		if (!primary.AllocatePrimaryFramebuffer(priProps))
		{
			UartPrintf("AllocatePrimaryFramebuffer fail\n");
			break;
		}
		priProps.mStride		= primary.GetFramebufferProperties().mStride;
		primary.SetSurfaceSelection(ePrimaryFront);

		// Fill the frame buffer by hand, which hopefully means we did it correctly :)
		Color32 priColor(0, 255, 0, eOpaque);
		for (int16_t y=priProps.mGeometry.y; y<(priProps.mGeometry.y + priProps.mGeometry.h); y++)
		{
			Color32* ptr = primary.GetSelectedFramebuffer() + priProps.mGeometry.x + y * priProps.mStride/4;
			for (int16_t x=0; x<priProps.mGeometry.w; x++)
			{
				*ptr++ = priColor;
			}
		}

		// Fill a box using FillRectangle and make sure if hits the right pixels
		UartPrintf("RunGeometryTests : FillRectangle() tests...\n");
		bool fail = false;
		Color32 fillColor(0, 0, 255, eOpaque);
		Rect rect(16, 64, 32, 32);
		primary.FillRectangle(rect, fillColor);
		for (int16_t y=priProps.mGeometry.y; y<(priProps.mGeometry.y + priProps.mGeometry.h); y++)
		{
			Color32* ptr = primary.GetSelectedFramebuffer() + y * priProps.mStride/4;
			for (int16_t x=priProps.mGeometry.x ; x<(priProps.mGeometry.x + priProps.mGeometry.w); x++)
			{
				if ((x >= rect.x) && (x < (rect.x + rect.w)) && (y >= rect.y) && (y < (rect.y + rect.h)))
				{
					if (*ptr != fillColor)
					{
						UartPrintf("fillMissing @ %d,%d ", x , y);
						fail = true;
					}
				}
				else
				{
					if (*ptr != priColor)
					{
						UartPrintf("fillSpillover @ %d,%d ", x , y);
						fail = true;
					}
				}
				ptr++;
			}
		}

		if (fail)
		{
			UartPrintf("FillRectangle fail\n");
			break;
		}

		UartPrintf("RunGeometryTests : GradientRectangle() tests...\n");
		std::vector<GradientStop> gradientStops;
		GradientStop stop;
		stop.mColor = Color32(0,0,0,eOpaque);
		stop.mPosition = 0.0f;
		gradientStops.push_back(stop);
		stop.mColor = Color32(63,127,255,eOpaque);
		stop.mPosition = 1.0f;
		gradientStops.push_back(stop);
		rect = Rect(16, 32, 64, 96);

		// Re-fill the primary buffer with the base color
		primary.FillRectangle(priProps.mGeometry, priColor);
		primary.GradientRectangle(rect, 0, gradientStops);

		for (int16_t y=priProps.mGeometry.y; y<(priProps.mGeometry.y + priProps.mGeometry.h); y++)
		{
			Color32* ptr = primary.GetSelectedFramebuffer() + y * priProps.mStride/4;
			for (int16_t x=priProps.mGeometry.x ; x<(priProps.mGeometry.x + priProps.mGeometry.w); x++)
			{
				if ((x >= rect.x) && (x < (rect.x + rect.w)) && (y >= rect.y) && (y < (rect.y + rect.h)))
				{
					if (*ptr == priColor)
					{
						UartPrintf("fillMissing @ %d,%d ", x , y);
						fail = true;
					}
				}
				else
				{
					if (*ptr != priColor)
					{
						UartPrintf("fillSpillover @ %d,%d ", x , y);
						fail = true;
					}
				}
				ptr++;
			}
		}

		if (fail)
		{
			UartPrintf("GradientRectangle fail\n");
			break;
		}

		res = true;

	} while (false);

	if (res)
		UartPrintf("RunFramebufferTests : passed\n");
	else
		UartPrintf("RunFramebufferTests : failed\n");
	
	return res;
}
