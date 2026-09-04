#include "Scanner.h"

void Scanner::setup(int width, int height) {
	camera.setup(width, height);
}

void Scanner::update() {
	if (mode != ScanMode::CAMERA) return;

	camera.update();
	if (camera.isFrameNew()) {
		std::string result = decoder.decode(camera.getPixels());
		if (!result.empty()) {
			lastDecoded = result;
		}
	}
}

void Scanner::draw(float x, float y, float w, float h) {
	if (mode == ScanMode::CAMERA) {
		camera.draw(x, y, w, h);
	} else {
		loadedImage.draw(x, y, w, h);
	}
}

void Scanner::setModeCamera() {
	mode = ScanMode::CAMERA;
}

bool Scanner::loadImageFile(const std::string & path) {
	if (!loadedImage.load(path)) {
		return false;
	}
	mode = ScanMode::FILE;

	std::string result = decoder.decode(loadedImage.getPixels());
	if (!result.empty()) {
		lastDecoded = result;
	}
	return true;
}

std::string Scanner::getLastDecoded() const {
	return lastDecoded;
}

bool Scanner::hasDecoded() const {
	return !lastDecoded.empty();
}
