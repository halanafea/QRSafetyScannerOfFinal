#pragma once
#include "ofMain.h"
#include "quirc/quirc.h"

class QrDecoder {
public:
	QrDecoder();
	~QrDecoder();

	std::string decode(const ofPixels & pixels);

private:
	struct quirc * qr = nullptr;
	int currentWidth = 0;
	int currentHeight = 0;
};
