#pragma once
#include "QrDecoder.h"
#include "ofMain.h"

enum class ScanMode { CAMERA,
	FILE };

class Scanner {
public:
	void setup(int width, int height);
	void update(); // call every frame - only does real work in CAMERA mode
	void draw(float x, float y, float w, float h);

	void setModeCamera();
	bool loadImageFile(const std::string & path); // returns false if load failed

	std::string getLastDecoded() const;
	bool hasDecoded() const;
	bool isCameraAvailable() const;

private:
	QrDecoder decoder; // Scanner owns its QrDecoder - composition
	ofVideoGrabber camera;
	ofImage loadedImage;
	ScanMode mode = ScanMode::CAMERA;
	std::string lastDecoded;
	bool cameraAvailable = false;
};
