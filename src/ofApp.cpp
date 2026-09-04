#include "ofApp.h"
#include <curl/curl.h>
#include <iostream>

size_t writeCallback(void * contents, size_t size, size_t nmemb, std::string * output) {
	size_t totalSize = size * nmemb;
	output->append(static_cast<char *>(contents), totalSize);
	return totalSize;
}

void ofApp::setup() {
	ofSetWindowTitle("QR Safety Scanner");
	ofBackground(245);
	ofSetWindowShape(460, 480);

	scanner.setup(640, 480);

	toggleButtons = {
		{ "Camera", ofRectangle(300, 15, 70, 26), true },
		{ "File", ofRectangle(375, 15, 60, 26), false },
	};

	// Temporary libcurl diagnostic test - DO NOT SHIP with SSL verification disabled
	CURL * curl = curl_easy_init();
	if (curl) {
		std::string response;
		curl_easy_setopt(curl, CURLOPT_URL, "https://example.com");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
		curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NO_REVOKE);

		// DIAGNOSTIC ONLY - disables certificate verification to isolate the cause.
		// Must be removed before this code is considered final.
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

		CURLcode res = curl_easy_perform(curl);
		if (res == CURLE_OK) {
			std::cout << "libcurl test succeeded. Response length: " << response.length() << " bytes" << std::endl;
		} else {
			std::cout << "libcurl test failed: " << curl_easy_strerror(res) << std::endl;
		}
		curl_easy_cleanup(curl);
	}
}

void ofApp::update() {
	scanner.update();
}

void ofApp::draw() {
	drawHeader();
	drawPreview();
	drawResultBanner();
}

void ofApp::drawHeader() {
	ofSetColor(30);
	ofDrawBitmapString("QR safety scanner", 20, 30);

	for (const auto & btn : toggleButtons) {
		bool active = (btn.isCameraButton == isCameraMode);
		ofSetColor(active ? ofColor(29, 158, 117) : ofColor(230));
		ofDrawRectRounded(btn.bounds, 6);

		ofSetColor(active ? ofColor(255) : ofColor(90));
		ofDrawBitmapString(btn.label, btn.bounds.x + 10, btn.bounds.y + 17);
	}
}

void ofApp::drawPreview() {
	ofSetColor(4, 52, 44);
	ofDrawRectangle(previewX, previewY, previewW, previewH);

	ofSetColor(255);
	scanner.draw(previewX, previewY, previewW, previewH);

	ofSetColor(93, 202, 165);
	ofSetLineWidth(3);
	float bx = previewX + previewW * 0.28f;
	float by = previewY + previewH * 0.26f;
	float bw = previewW * 0.44f;
	float bh = previewH * 0.44f;
	float corner = 18;

	ofDrawLine(bx, by, bx + corner, by);
	ofDrawLine(bx, by, bx, by + corner);
	ofDrawLine(bx + bw, by, bx + bw - corner, by);
	ofDrawLine(bx + bw, by, bx + bw, by + corner);
	ofDrawLine(bx, by + bh, bx + corner, by + bh);
	ofDrawLine(bx, by + bh, bx, by + bh - corner);
	ofDrawLine(bx + bw, by + bh, bx + bw - corner, by + bh);
	ofDrawLine(bx + bw, by + bh, bx + bw, by + bh - corner);

	if (!scanner.hasDecoded()) {
		ofSetColor(159, 225, 203);
		ofDrawBitmapString("Point your camera at a QR code", previewX + 20, previewY + previewH - 15);
	}
}

void ofApp::drawResultBanner() {
	float bannerY = previewY + previewH + 15;
	float bannerH = 55;

	if (!scanner.hasDecoded()) {
		ofSetColor(240, 240, 235);
		ofDrawRectRounded(previewX, bannerY, previewW, bannerH, 8);
		ofSetColor(120);
		ofDrawBitmapString("No code scanned yet", previewX + 15, bannerY + 25);
		ofSetColor(160);
		ofDrawBitmapString("Results will appear here", previewX + 15, bannerY + 42);
	} else {
		ofSetColor(235, 235, 225);
		ofDrawRectRounded(previewX, bannerY, previewW, bannerH, 8);
		ofSetColor(90);
		ofDrawBitmapString("Scanned - safety check pending", previewX + 15, bannerY + 25);
		ofSetColor(120);
		ofDrawBitmapString(scanner.getLastDecoded(), previewX + 15, bannerY + 42);
	}
}

void ofApp::mousePressed(int x, int y, int button) {
	for (const auto & btn : toggleButtons) {
		if (btn.bounds.inside(x, y)) {
			if (btn.isCameraButton) {
				isCameraMode = true;
				scanner.setModeCamera();
			} else {
				ofFileDialogResult result = ofSystemLoadDialog("Select a QR code image");
				if (result.bSuccess) {
					isCameraMode = false;
					if (!scanner.loadImageFile(result.getPath())) {
						ofLogError() << "Failed to load selected image";
					}
				}
			}
		}
	}
}

void ofApp::keyPressed(int key) { }
void ofApp::keyReleased(int key) { }
void ofApp::mouseMoved(int x, int y) { }
void ofApp::mouseDragged(int x, int y, int button) { }
void ofApp::mouseReleased(int x, int y, int button) { }
void ofApp::mouseEntered(int x, int y) { }
void ofApp::mouseExited(int x, int y) { }
void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY) { }
void ofApp::windowResized(int w, int h) { }
void ofApp::dragEvent(ofDragInfo dragInfo) { }
void ofApp::gotMessage(ofMessage msg) { }
