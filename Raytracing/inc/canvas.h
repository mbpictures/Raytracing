#pragma once

#include <nanogui/nanogui.h>
#include <stdint.h>

NAMESPACE_BEGIN(nanogui)

struct Color3 {
	double r;
	double g;
	double b;
};

class Canvas2D : ImageView {
	friend class ImageView;
public:
	Canvas2D(Widget *parent, int x, int y);


	void drawPixel(double r, double g, double b, int x, int y);
	void clearDisplay();
private:
	void updateImage();
	int imageSizeX;
	int imageSizeY;
	typedef unsigned __int8 u8;
	u8 ***screenData;
	

public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};

NAMESPACE_END(nanogui)