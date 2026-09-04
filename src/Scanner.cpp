#include "Scanner.h"

void Scanner::setup(int width, int height) {
	camera.setup(width, height);
	// isInitialized() is openFrameworks' documented way to check this. Note:
	// some video backends are known to report true even without a real
	// camera present - not airtight, but File mode remains available as a
	// fallback regardless, so this is an acceptable heuristic for this
	// project's scope (see documentation Section 16, Phase 7).
	cameraAvailable = camera.isInitialized();
}

void Scanner::update() {
	if (mode != ScanMode::CAMERA || !cameraAvailable) return;

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
		if (cameraAvailable) {
			camera.draw(x, y, w, h);
		}
		// If unavailable, draw nothing here - ofApp overlays a message on
		// top of the preview area's existing background instead.
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

	// Unlike camera mode's frame-by-frame update (where we deliberately keep
	// showing the last successful decode across frames that don't have a
	// code in view), loading a file is one discrete action - so the result
	// should always reflect this image specifically, including clearing any
	// previous decode if this one has no QR code at all.
	lastDecoded = decoder.decode(loadedImage.getPixels());
	return true;
}

std::string Scanner::getLastDecoded() const {
	return lastDecoded;
}

bool Scanner::hasDecoded() const {
	return !lastDecoded.empty();
}

bool Scanner::isCameraAvailable() const {
	return cameraAvailable;
}
