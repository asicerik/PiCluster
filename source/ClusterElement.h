#pragma once

#include "Region.h"

class ClusterElement
{
public:
	ClusterElement();
	virtual ~ClusterElement();

	virtual bool Init(const Rect& box, bool isPrimary=false, bool doubleBuffer=false);
	virtual void SetVisible(bool visible);
	virtual bool GetVisible()		{ return mVisible; };

	// Invalidate the area of this element indicated by the rectangle.
	// NOTE : 'box' is in screen coordinates
	virtual void Invalidate(const Rect& box);
	
	// Invalidate the area of this element indicated by the region.
	// NOTE : 'region' is in screen coordinates
	virtual void Invalidate(const Region& region);

	// Set the location of this element
	virtual void SetLocation(const Point& loc);

	virtual Region Update();
	virtual Region Draw();
	virtual void   DrawLabel();
	virtual void   DrawLabelImage();
	
	void AddGradientStop(float position, uint8_t a, uint8_t r, uint8_t g, uint8_t b);
	void SetGradientAngle(int16_t angle);
	GraphicsContext& GetGraphicsContext()	{ return mGfx; };

	Rect GetClientRect()					{ return mClientRect; };

	void SetLabelText(std::string label)	{ mLabelText = label; };
	void SetLabelCenter(Point loc)			{ mLabelCenter = loc; };
	void SetTextColor(Color32 color)		{ mTextColor = color; };
	void SetLabelImage(BMPImage* image)		{ mLabelImage = image; };
	void SetImage(BMPImage* image)			{ mImage = image; };
	void SetImageAlpha(uint8_t alpha)		{ mImageAlpha = alpha; };
	void SetLabelImageCenter(Point loc)		{ mLabelImageCenter = loc; };

protected:
	bool				mVisible;

	// NOTE : the "ALIGN" below. ARM does not like un-aligned memory access
	Rect				mClientRect 	ALIGN;		//!< The bounds for this element in client space (upper left is 0,0)
	GraphicsContext		mGfx;
	Region				mForegroundDirtyRegion;		//!< The portion of our foreground that needs to be redrawn
	Region				mBackgroundDirtyRegion;		//!< The portion of our background that needs to be redrawn
	Region				mScreenDirtyRegion;			//!< The portion of the object below us that needs to be redrawn
													//!< NOTE : this is in screen coordinates, not client
	Region				mTempRegion;				//!< Temp storage region. Never assume this contains anything valid!
	std::vector<GradientStop>
						mGradientStops;
	bool				mStateChanged;
	int16_t				mGradientAngle 	ALIGN;
	std::string			mLabelText;					//!< The text label for this element
	Point				mLabelCenter;				//!< The location of the center of the label
	Color32				mTextColor;					//!< Color for text - labels, etc.
	BMPImage*			mLabelImage;				//!< The label image for this element
	Point				mLabelImageCenter;			//!< The location of the center of the label image
	BMPImage*			mImage;						//!< The image for this element (if any)
	uint8_t				mImageAlpha;				//!< Global alpha value to apply to image, or eOpaque
	int16_t				mImageAlphaCurrent;			//!< If we are fading in/out, this is the current val
};


