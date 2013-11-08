#pragma once

#include <vector>
#include <string>
#include "dma.h"
#include "Timer.h"

static const uint32_t kMaxFramebufferWidth	= 4096;
static const uint32_t kMaxFramebufferHeight	= 4096;
static const uint32_t kMaxFramebufferDepth	= 32;

#define SATURATE_II(x) (((x) > 255) ? 255 : ((x) < 0) ? 0 : (x))
#define SATURATE_FF(x) (((x) > 255) ? 255 : ((x) < 0) ? 0 : (x))
#define SATURATE_FI(x) (((x) > 255.0) ? 255 : ((x) < 0.0) ? 0 : (uint8_t)(x))

#define Clip(x,min,max) ((x)<(min)?(min):((x)>(max)?(max):(x)))

struct PACK Point
{
	Point() {};

	// The default copy constructor does not seem to work in Pi-land, so...
	Point(const Point& in)
	{
		x = in.x;
		y = in.y;
	}
	Point(int16_t xIn, int16_t yIn)
	{
		x = xIn;
		y = yIn;
	}
	Point& operator=(const Point &rhs)
	{
	    // Check for self-assignment!
	    if (this == &rhs)      // Same object?
	      return *this;        // Yes, so skip assignment, and just return *this.

	    this->x = rhs.x;
	    this->y = rhs.y;
	    return *this;
	}
	int16_t		x;
	int16_t		y;
};

struct PACK Rect
{
	Rect() {};
	// The default copy constructor does not seem to work in Pi-land, so...
	Rect(const Rect& in)
	{
		x = in.x;
		y = in.y;
		w = in.w;
		h = in.h;
	}
	Rect(int16_t xIn, int16_t yIn, int16_t wIn, int16_t hIn)
	{
		x = xIn;
		y = yIn;
		w = wIn;
		h = hIn;
	}
	Rect& operator=(const Rect &rhs)
	{
	    // Check for self-assignment!
	    if (this == &rhs)      // Same object?
	      return *this;        // Yes, so skip assignment, and just return *this.

	    this->x = rhs.x;
	    this->y = rhs.y;
	    this->w = rhs.w;
	    this->h = rhs.h;
	    return *this;
	}
	int16_t		x;
	int16_t		y;
	int16_t		w;
	int16_t		h;
};

struct Color32f;

#ifdef WIN32
struct Color32
{
	Color32(uint8_t	rIn, uint8_t gIn, uint8_t bIn, uint8_t aIn)
	{
		r = rIn;
		g = gIn;
		b = bIn;
		a = aIn;
	};
	Color32() {};
	uint8_t		b;
	uint8_t		g;
	uint8_t		r;
	uint8_t		a;
	Color32f ToColor32f();
	inline Color32	 AlphaBlend(Color32 background)
	{
		Color32 ret;
		// Special cases
		if (a == 255)
			return *this;
		if (a == 0)
			return background;

#ifdef PRE_MULT
		uint32_t inv_alpha = 256 - a;
		ret.r = (unsigned char)((r + inv_alpha * background.r) >> 8);
		ret.g = (unsigned char)((g + inv_alpha * background.g) >> 8);
		ret.b = (unsigned char)((b + inv_alpha * background.b) >> 8);
		ret.a = 255;
#else
		uint32_t alpha = a + 1;
		uint32_t inv_alpha = 256 - a;
		ret.r = (unsigned char)((alpha * r + inv_alpha * background.r) >> 8);
		ret.g = (unsigned char)((alpha * g + inv_alpha * background.g) >> 8);
		ret.b = (unsigned char)((alpha * b + inv_alpha * background.b) >> 8);
		ret.a = 255;
#endif
		//uint8_t backgroundAlpha = 255 - a;
		//ret.r = SATURATE_II(((uint16_t)r * a + (uint16_t)background.r * backgroundAlpha) >> 8);
		//ret.g = SATURATE_II(((uint16_t)g * a + (uint16_t)background.g * backgroundAlpha) >> 8);
		//ret.b = SATURATE_II(((uint16_t)b * a + (uint16_t)background.b * backgroundAlpha) >> 8);
		//ret.a = 255;
		return ret;
	}
	inline Color32	 AlphaBlend(Color32 background, uint8_t globalAlpha)
	{
		Color32 ret;
		uint32_t alpha = a;
		alpha = (alpha > globalAlpha ? globalAlpha : alpha) + 1;

		// Special cases
		if (alpha == 256)
			return *this;
		if (alpha == 1)
			return background;

#ifdef PRE_MULT
		uint32_t inv_alpha = 256 - (alpha - 1);
		ret.r = (unsigned char)((r + inv_alpha * background.r) >> 8);
		ret.g = (unsigned char)((g + inv_alpha * background.g) >> 8);
		ret.b = (unsigned char)((b + inv_alpha * background.b) >> 8);
		ret.a = 255;
#else
		uint32_t inv_alpha = 256 - (alpha - 1);
		ret.r = (unsigned char)((alpha * r + inv_alpha * background.r) >> 8);
		ret.g = (unsigned char)((alpha * g + inv_alpha * background.g) >> 8);
		ret.b = (unsigned char)((alpha * b + inv_alpha * background.b) >> 8);
		ret.a = 255;
#endif
		//uint8_t backgroundAlpha = 255 - a;
		//ret.r = SATURATE_II(((uint16_t)r * a + (uint16_t)background.r * backgroundAlpha) >> 8);
		//ret.g = SATURATE_II(((uint16_t)g * a + (uint16_t)background.g * backgroundAlpha) >> 8);
		//ret.b = SATURATE_II(((uint16_t)b * a + (uint16_t)background.b * backgroundAlpha) >> 8);
		//ret.a = 255;
		return ret;
	}
	bool operator==(const Color32 &rhs)
	{
		if ((this->a == rhs.a) && (this->r == rhs.r) && (this->g == rhs.g) && (this->b == rhs.b))
			return true;
		else
			return false;
	}
	bool operator!=(const Color32 &rhs)
	{
		if ((this->a == rhs.a) && (this->r == rhs.r) && (this->g == rhs.g) && (this->b == rhs.b))
			return false;
		else
			return true;
	}
};
#else
struct PACK Color32
{
	Color32(uint8_t	rIn, uint8_t gIn, uint8_t bIn, uint8_t aIn)
	{
		r = rIn;
		g = gIn;
		b = bIn;
		a = aIn;
	};
	Color32() {};
	uint8_t		r;
	uint8_t		g;
	uint8_t		b;
	uint8_t		a;
	Color32f ToColor32f();
	inline Color32	 AlphaBlend(Color32 background)
	{
		Color32 ret;
		// Special cases
		if (a == 255)
			return *this;
		if (a == 0)
			return background;

		uint8_t backgroundAlpha = 255 - a;
		ret.r = SATURATE_II(((uint16_t)r * a + (uint16_t)background.r * backgroundAlpha) >> 8);
		ret.g = SATURATE_II(((uint16_t)g * a + (uint16_t)background.g * backgroundAlpha) >> 8);
		ret.b = SATURATE_II(((uint16_t)b * a + (uint16_t)background.b * backgroundAlpha) >> 8);
		ret.a = 255;
		return ret;
	}
	inline Color32	 AlphaBlend(Color32 background, uint8_t globalAlpha)
	{
		Color32 ret;
		uint32_t alpha = a;
		alpha = (alpha > globalAlpha ? globalAlpha : alpha) + 1;

		// Special cases
		if (alpha == 256)
			return *this;
		if (alpha == 1)
			return background;

		uint32_t inv_alpha = 256 - (alpha - 1);
		ret.r = (unsigned char)((alpha * r + inv_alpha * background.r) >> 8);
		ret.g = (unsigned char)((alpha * g + inv_alpha * background.g) >> 8);
		ret.b = (unsigned char)((alpha * b + inv_alpha * background.b) >> 8);
		ret.a = 255;
		//uint8_t backgroundAlpha = 255 - a;
		//ret.r = SATURATE_II(((uint16_t)r * a + (uint16_t)background.r * backgroundAlpha) >> 8);
		//ret.g = SATURATE_II(((uint16_t)g * a + (uint16_t)background.g * backgroundAlpha) >> 8);
		//ret.b = SATURATE_II(((uint16_t)b * a + (uint16_t)background.b * backgroundAlpha) >> 8);
		//ret.a = 255;
		return ret;
	}

	inline void	SwapRGBChannels()
	{
		uint8_t temp = r;
		r = b;
		b = temp;
	}

	inline bool CompareSansAlpha(const Color32 in)
	{
		if ((r == in.r) && (g == in.g) && (b == in.b))
			return true;
		else
			return false;
	}
	bool operator==(const Color32 &rhs)
	{
		if ((this->a == rhs.a) && (this->r == rhs.r) && (this->g == rhs.g) && (this->b == rhs.b))
			return true;
		else
			return false;
	}
	bool operator!=(const Color32 &rhs)
	{
		if ((this->a == rhs.a) && (this->r == rhs.r) && (this->g == rhs.g) && (this->b == rhs.b))
			return false;
		else
			return true;
	}
};
#endif

struct PACK Color32f
{
	float		a;
	float		b;
	float		g;
	float		r;
	Color32 ToColor32();
};

// A color point in x/y space
struct PACK GradientStop
{
	float		mPosition;
	Color32		mColor;
};

enum Opacity
{
	eTransparent	= 0,
	e25Percent		= 64,
	e50Percent		= 128,
	e75Percent		= 192,
	eOpaque			= 255,
};

/*
	AntiAliasEdges are used to decide which edges to apply anti-aliasing to.
	Sometimes, we may want to turn on/off anti-aliasing for certain edges.
	Especially on butt joints between polygons.
*/
enum AntiAliasEdges
{
	eAntiAliasNone		= 0,	// no edges are to be anti-aliased
	eAntiAliasS0		= 0x1,	// left-side only
	eAntiAliasS1		= 0x2,	// top only
	eAntiAliasS2		= 0x4,	// right-side only
	eAntiAliasS3		= 0x8,	// bottom only
	eAntiAliasS0S2		= 0x5,	// left/right only
	eAntiAliasS1S3		= 0xa,	// top/bottom only
	eAntiAliasS0S1S2S3	= 0xf,	// all four sides
};

/*
	AntiAliasLineMode is used to determine the mode used to anti-alias drawn lines.
	In the simple alpha-reduction mode, we draw the line with single-pixel width,
	but modulate the alpha channel according to the error from ideal.
	You need to pick a direction for the alpha blending depending on where the line
	is relative to filled regions, etc.
	If you choose the wrong value, you will see hard edges instead of smooth.
*/
enum AntiAliasLineMode
{
	eAntiAliasLineModeNone		= 0,	// don't apply anti-aliasing to this line
	eAntiAliasLineModeAlpha1	= 1,	// use anti-aliasing alpha mode 1
	eAntiAliasLineModeAlpha2	= 2,	// use anti-aliasing alpha mode 2
};

enum SurfaceSelection
{
	eUnknown,
	ePrimaryFront,
	ePrimaryBack,
	eFront,
	eBack
};

struct ALIGN FramebufferProperties
{
	FramebufferProperties()
	{
		mBitsPerPixel 	= 32;
		mStride			= 0;
		mDoubleBuffer	= false;
		mBorderSize		= 0;
	}
	Rect			mGeometry;
	int8_t			mBitsPerPixel;		//!< Only 32 is valid currently
	int32_t			mStride;			//!< Stride in bytes
	bool			mDoubleBuffer;		//!< Allocate a front and back buffer for drawing
	int16_t			mBorderSize;		//!< Number of pixels to pad each border by - TEST ONLY!
};

/*
	FontDatabaseFile
	This file is used to store an anti-aliased font.
	The only storage is the alpha channel. To write text using this font, you simply 
	apply this alpha channel to your chosen text color and alpha blend it into your framebuffer.
*/
struct PACK FontDatabaseFile
{
	uint32_t	fileSize;				//!< Total filesize including this header
	char		fontName[32];			//!< NULL terminated font name. Max 31 characters
	uint8_t		fontHeight;				//!< Font height
	uint8_t		fontAscent;				//!< Number of pixels above the baseline
	uint8_t		fontDescent;			//!< Number of pixels below the baseline
	uint8_t		startChar;				//!< First ASCII character in the file (typically 32=space)
	uint8_t		endChar;				//!< Last ASCII character in the file (typically 126=~)
	uint8_t		columns;				//!< The number of characters per line
	uint8_t		rows;					//!< The number of lines
	uint8_t		cellWidth;				//!< The width of each character cell in the image
	uint8_t		cellHeight;				//!< The width of each character cell in the image
	uint8_t		padding1;				//!< For alignment
	uint8_t		padding2;				//!< For alignment
	uint8_t		widthArray[256];		//!< The drawn width of each char (starting from "startChar", not 0
	uint8_t		alphaArray;				//!< This is actually an array of alpha values. Size = (endChar-startChar)+1
										//!< 255 = fully opaque. 0 = transparent (background)
};

enum TextAlignment
{
	eAlignLeft,
	eAlignRight,
	eAlignCenter
};

// BMP file format
// We use the BMP format for our images since it is simple and has 32 bit ARGB capability
struct PACK BMPImage
{
	char		id[2];		// Should be "BM"
	int32_t		bmpSize;	// Size of total file
	int16_t		reserved1;
	int16_t		reserved2;
	int32_t		offset;		// Offset to pixel array
	// DIB header
	int32_t		headerSize;	// DIB header size
	int32_t		width;		// Image width
	int32_t		height;		// Image height
	int16_t		planes;		// Number of image planes
	int16_t		bpp;		// Bit per pixel
	int32_t		compression;// BI_RGB is all we support
	int32_t		arraySize;	// Number of bytes in pixel array
	int32_t		horizRes;	// Horizontal resolution
	int32_t		vertRes;	// Vertical resolution
	int32_t		paletteColors; // Number of colors in palette
	int32_t		impColors;	// Important colors
	Color32		pixelArray;	// Start of pixel array
};

// ProfileData is used to profile the code and measure where the bottlenecks are
struct ProfileData
{
	ProfileData()	{ Clear();	}
	void Clear()
	{
		mUpdate.Reset();
		mDraw.Reset();
		mCopyToPrimary.Reset();
		mCopyBackToFront.Reset();
		mDMA.Reset();
		mFillRectangle.Reset();
		mGradientLine.Reset();
		mGradientRectangle.Reset();
		mDrawLine.Reset();
		mDrawTrapezoid.Reset();
		mDrawArc.Reset();
		mFloodFill.Reset();
		mDrawText.Reset();
		mRegion.Reset();
	}
	void Snapshot(bool clear)
	{
		mUpdate.Snapshot(clear);
		mDraw.Snapshot(clear);
		mCopyToPrimary.Snapshot(clear);
		mCopyBackToFront.Snapshot(clear);
		mDMA.Snapshot(clear);
		mFillRectangle.Snapshot(clear);
		mGradientLine.Snapshot(clear);
		mGradientRectangle.Snapshot(clear);
		mDrawLine.Snapshot(clear);
		mDrawTrapezoid.Snapshot(clear);
		mDrawArc.Snapshot(clear);
		mFloodFill.Snapshot(clear);
		mDrawText.Snapshot(clear);
		mRegion.Snapshot(clear);
	}

	// These are all cumulative times (in usec) spent in each routine
	Timer	mUpdate;
	Timer	mDraw;
	Timer	mCopyToPrimary;
	Timer	mCopyBackToFront;
	Timer	mDMA;
	Timer	mFillRectangle;
	Timer	mGradientLine;
	Timer	mGradientRectangle;
	Timer	mDrawLine;
	Timer	mDrawTrapezoid;
	Timer	mDrawArc;
	Timer	mFloodFill;
	Timer	mDrawText;
	Timer	mRegion;
};

class Region;
class GraphicsContextBase
{
public:
	static Color32	kMagenta;

	GraphicsContextBase();
	virtual ~GraphicsContextBase() {};

	virtual bool	AllocatePrimaryFramebuffer(FramebufferProperties& properties) = 0;
	virtual bool	AllocateFramebuffer(FramebufferProperties& properties) = 0;
	virtual void	FreeFramebuffer() = 0;
	virtual void	FillRectangle(Rect rect, Color32 argb);
	static void		PrepareBMP(BMPImage* image);
	virtual void	DrawBMP(Point& dstLoc, BMPImage* image, bool alphaBlend);
	virtual void	CopyToPrimary(std::vector<Rect> rects, bool alphaBlend=false);
	virtual void	CopyBackToFront(Rect& rect);
	virtual void	GradientLine(Color32 startColor, Color32f colorDelta, int16_t startX, int16_t endX, int16_t y);
	virtual void	GradientRectangle(Rect location, int16_t angle, std::vector<GradientStop>& stops);
	virtual void	DrawLine(Color32 color, int16_t x0, int16_t y0, int16_t x1, int16_t y1, 
							 AntiAliasLineMode aaMode=eAntiAliasLineModeNone);
	virtual void	DrawLine(Color32 color, int16_t x0, int16_t y0, int16_t x1, int16_t y1,
							 Point* points, int16_t& pointsSize, 
							 AntiAliasLineMode aaMode=eAntiAliasLineModeNone);
	virtual void	DrawTrapezoid(Color32 color, Point origin, int32_t angleWide, int16_t innerRadius, 
								  int16_t outerRadius, int32_t startArcWide, int32_t endArcWide, bool fill,
								  AntiAliasEdges aaEdges);
	virtual void	DrawArc(Color32 color, Point origin, int16_t startAngleWide, int16_t endAngleWide, int16_t radius);
	virtual void	DrawCircle(Color32 color, Point origin, int16_t radius, bool fill);
	virtual void	FloodFill(int16_t x, int16_t y, uint32_t borderColor, uint32_t fillColor);
	virtual void	DrawEmboss(Color32 colorMax, Point origin, int16_t innerRadius, int16_t outerRadius, 
					  int32_t startAngleWide, int32_t endAngleWide, int32_t peakAngleWide, int32_t stepAngleWide=20);
	virtual void	WaitForVSync()	{};

	// Text functions
	virtual int16_t	GetTextDrawnLength(FontDatabaseFile* font, char* text);
	virtual void	DrawText(FontDatabaseFile* font, char* text, Point& loc, Color32& color, bool alphaBlend, TextAlignment align);
	virtual int16_t	GetTextDrawnLength(FontDatabaseFile* font, std::string& text);
	virtual void	DrawText(FontDatabaseFile* font, std::string& text, Point& loc, Color32& color, bool alphaBlend, TextAlignment align);
	
	void SetClippingRect(Rect& rect)	{ mClippingRect = rect; };
	Rect GetClippingRect()				{ return mClippingRect; };

	GraphicsContextBase*	SetSurfaceSelection(SurfaceSelection selection);	
	SurfaceSelection		GetSurfaceSelection()		{ return mSurfaceSelection; };	
	GraphicsContextBase*	GetSelectedSurface()		{ return mSelectedSurface; };
	Color32*				GetSelectedFramebuffer()	{ return mCurrBufferPtr; };
	Color32*				GetBackBuffer()				{ return mBackBufferPtr; };
	Color32*				GetFrontBuffer()			{ return mFrontBufferPtr; };
	void					SetScreenOffset(Point offset) { mScreenOffset = offset; };
	Point					GetScreenOffset()			{ return mScreenOffset; };
	Point					ClientToScreen(const Point& clientPos);
	Rect					ClientToScreen(const Rect&  clientRect);
	Point					ScreenToClient(const Point& screenPos);
	Rect					ScreenToClient(const Rect&  screenRect);
	void					SetAntiAlias(bool val)		{ mAntiAlias = val; };
	bool					GetAntiAlias()				{ return mAntiAlias; };
	void					EnableDirtyRects(Region* region)	{ mDirtyRegion = region; };
	void					DisableDirtyRects()			{ mDirtyRegion = NULL; };
	void					SetGlobalAlpha(uint8_t a)	{ mGlobalAlpha = a; };
	uint8_t					GetGlobalAlpha(uint8_t a)	{ return mGlobalAlpha; };

	const FramebufferProperties& GetFramebufferProperties()	{ return mFBProperties; };

	// Profiling
	static ProfileData		mProfileData;			//!< Storage for our profiling info

	// Test stuff
	bool					mTestMode;				//!< If true, apply test settings

protected:
	virtual void	FloodFillLeft(uint32_t intColor, Point* start, int16_t arraySize);
	virtual void	FloodFillRight(uint32_t intColor, Point* start, int16_t arraySize);

	int32_t					mALign	ALIGN;			//!< Make sure everything below is 32b aligned
	static GraphicsContextBase*	mPrimaryContext;	//!< Pointer to the primary surface
	GraphicsContextBase*	mSelectedSurface;		//!< Pointer to the drawing surface
	SurfaceSelection		mSurfaceSelection;		//!< Which surface are we drawing to?
	Color32*				mCurrBufferPtr;			//!< Pointer to the currently selected framebuffer data
	Color32*				mFrontBufferPtr;		//!< Pointer to the active framebuffer data
	Color32*				mBackBufferPtr;			//!< Pointer to the inactive framebuffer data (if present)
	FramebufferProperties	mFBProperties;			//!< Properties for this framebuffer
	Rect					mClippingRect;			//!< Clipping rectangle from drawing functions
	Point					mScreenOffset;			//!> x/y offset relative to primary surface
	Region*					mDirtyRegion;			//!< If non-null, use this region to track dirty rects
	uint8_t					mGlobalAlpha;			//!< Global alpha to apply to alpha blending (or eOpaque)

	// Put all the bools/non-aligned stuff at the end
	bool					mAntiAlias;				//!< If true, apply anti-aliasing
};

#ifdef WIN32
class GraphicsContextWin : public GraphicsContextBase
{
public:
	virtual bool	AllocatePrimaryFramebuffer(FramebufferProperties& properties);
	virtual bool	AllocateFramebuffer(FramebufferProperties& properties);
	virtual void	FreeFramebuffer();
	virtual void	FillRectangle(Rect rect, Color32 argb);
	bool			CreateFontDatabase(std::string fontName, int16_t height);
	Color32* GetBackBuffer();
	Color32* GetFrontBuffer();
	HDC GetDC()	{ return mDC; };

protected:
	bool AllocateWindowsBitmap(FramebufferProperties& properties, HDC& dc, BITMAPINFO& info, HBITMAP& bitmap, Color32*& ptr);
	HDC						mDC;
	BITMAPINFO				mBitmapInfo;
	HBITMAP					mBitmap;
	HBITMAP					mBackBufferBitmap;
};
typedef GraphicsContextWin GraphicsContext;
#else
// VideoCore mailbox registers
static uint32_t* VideoCoreMailboxRead		= (uint32_t*)0x2000B880;
static uint32_t* VideoCoreMailboxPeek		= (uint32_t*)0x2000B890;
static uint32_t* VideoCoreMailboxSender		= (uint32_t*)0x2000B894;
static uint32_t* VideoCoreMailboxStatus		= (uint32_t*)0x2000B898;
static uint32_t* VideoCoreMailboxConfig		= (uint32_t*)0x2000B89C;
static uint32_t* VideoCoreMailboxWrite		= (uint32_t*)0x2000B8A0;

// Add this value to mailbox addresses to prevent the VPU from writing the
// result in a cache, which means we won't see it until the cache flushes
static uint32_t  kVideoCoreMailboxDisableCache = 0x40000000;

// VideoCore mailbox status register bits
// The actual bit is active low (0 = ready)
static const uint32_t VideoCoreMailboxStatusWriteReady	= 0x80000000;
static const uint32_t VideoCoreMailboxStatusReadReady	= 0x40000000;

// VideoCore mailbox channels
enum VideoCoreChannel
{
	eVCPowerManagementChannel,
	eVCFramebufferChannel,
	eVCVirtualUARTChannel,
	eVCVHCIQChannel,
	eVCLEDChannel,
	eVCButtonChannel,
	eVCTouchscreenChannel
};

// VideoCore mailbox functions
bool MailboxWrite(uintptr_t message, enum VideoCoreChannel channel);
bool MailboxRead(uintptr_t& message, enum VideoCoreChannel channel);

// VideoCore framebuffer structure
// NOTE : make sure you align your memory to a 16B boundary
struct PACK VideoCoreFramebufferDescriptor
{
	VideoCoreFramebufferDescriptor()
	{
		width			= 0;
		height			= 0;
		virtualWidth	= 0;
		virtualHeight	= 0;
		pitch			= 0;
		depth			= 0;
		xOffset			= 0;
		yOffset			= 0;
		buffer			= 0;
		bufferSize		= 0;
	}
	uint32_t	width;
	uint32_t	height;
	uint32_t	virtualWidth;
	uint32_t	virtualHeight;
	uint32_t	pitch;
	uint32_t	depth;
	uint32_t	xOffset;
	uint32_t	yOffset;
	uint32_t*	buffer;
	uint32_t	bufferSize;
}  __attribute__ ((aligned(16)));


class GraphicsContextPi : public GraphicsContextBase
{
public:
	GraphicsContextPi();
	virtual ~GraphicsContextPi();

	virtual bool	AllocatePrimaryFramebuffer(FramebufferProperties& properties);
	virtual bool	AllocateFramebuffer(FramebufferProperties& properties);
	virtual void	FreeFramebuffer();
	virtual void	FillRectangle(Rect rect, Color32 argb);
	virtual void	CopyBackToFront(Rect& rect);
	virtual void	WaitForVSync();

private:
	bool CreatePrimaryFramebuffer(VideoCoreFramebufferDescriptor& fbDesc);
};
typedef GraphicsContextPi GraphicsContext;


#endif
