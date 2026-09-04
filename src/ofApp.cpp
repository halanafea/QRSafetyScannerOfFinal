#include "ofApp.h"
#include "ApiKeyLoader.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace {
// Cross-platform browser opener, per the locked GUI design (documentation
// Section 12.3). Kept as a free function, not a Scanner/ofApp method,
// since it has nothing to do with either class's responsibilities.
void openInBrowser(const std::string & url) {
#ifdef _WIN32
	std::string command = "start " + url;
#elif __APPLE__
	std::string command = "open " + url;
#endif
	system(command.c_str());
}
}

std::string ofApp::truncateToFit(const std::string & text, float maxWidth) const {
	// ofDrawBitmapString uses a fixed-width bitmap font, ~8px per glyph.
	// Long decoded URLs (test/malicious URLs especially) can easily run
	// past the banner or dialog edge if drawn unclipped, so anything that
	// won't fit gets cut with an ellipsis rather than overflowing.
	const float charWidth = 8.0f;
	size_t maxChars = static_cast<size_t>(maxWidth / charWidth);

	if (text.size() <= maxChars) {
		return text;
	}
	if (maxChars <= 3) {
		return text.substr(0, maxChars);
	}
	return text.substr(0, maxChars - 3) + "...";
}

bool ofApp::looksLikeUrl(const std::string & text) const {
	// Deliberately conservative: only treat clearly URL-shaped text as
	// checkable. Anything else (plain text, phone numbers, vCards, Wi-Fi
	// configs, etc.) skips the safety check entirely, rather than sending
	// non-URL data to the Safe Browsing API and showing a verdict that
	// implies it was actually checked as a link.
	std::string lower = text;
	std::transform(lower.begin(), lower.end(), lower.begin(),
		[](unsigned char c) { return std::tolower(c); });
	return lower.rfind("http://", 0) == 0 || lower.rfind("https://", 0) == 0;
}

void ofApp::showError(const std::string & message) {
	errorPopupMessage = message;
	showErrorPopup = true;
}

void ofApp::setup() {
	ofSetWindowTitle("QR Safety Scanner");
	ofBackground(245);

	scanner.setup(640, 480);

	if (!scanner.isCameraAvailable()) {
		showError("No camera detected. Use File mode instead.");
	}

	toggleButtons = {
		{ "Camera", ofRectangle(300, 15, 70, 26), true },
		{ "File", ofRectangle(375, 15, 60, 26), false },
	};

	// Loaded once here rather than per-check; LinkSafetyChecker itself is
	// still constructed fresh in checkSafetyNow(), same pattern as the
	// Phase 5 test call.
	apiKey = ApiKeyLoader::load("apikey.txt");
}

void ofApp::update() {
	scanner.update();

	if (scanner.hasDecoded()) {
		std::string current = scanner.getLastDecoded();
		if (current != lastSeenDecodedUrl) {
			// A newly-decoded code invalidates any previous verdict - back
			// to PENDING (or NOT_A_LINK) until the user explicitly checks
			// this one.
			lastSeenDecodedUrl = current;
			bannerState = looksLikeUrl(current) ? BannerState::PENDING : BannerState::NOT_A_LINK;
			showConfirmDialog = false;
		}
	} else if (bannerState != BannerState::EMPTY) {
		// Scanner currently has nothing decoded (e.g. a freshly-loaded file
		// had no QR code in it) - don't keep showing a stale verdict from
		// whatever was decoded before.
		bannerState = BannerState::EMPTY;
		lastSeenDecodedUrl.clear();
		showConfirmDialog = false;
	}
}

void ofApp::draw() {
	drawHeader();
	drawPreview();
	drawResultBanner();

	if (showConfirmDialog) {
		drawConfirmDialog();
	}
	if (showErrorPopup) {
		drawErrorPopup();
	}
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

	// Scan-target corner brackets: teal by default, red once the last
	// checked result came back UNSAFE (locked design, Section 12.2).
	if (bannerState == BannerState::UNSAFE) {
		ofSetColor(214, 69, 65);
	} else {
		ofSetColor(93, 202, 165);
	}
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

	if (isCameraMode && !scanner.isCameraAvailable()) {
		ofSetColor(220, 140, 140);
		ofDrawBitmapString("No camera detected - use File mode", previewX + 20, previewY + previewH / 2);
	} else if (!scanner.hasDecoded()) {
		ofSetColor(159, 225, 203);
		std::string hint = isCameraMode
			? "Point your camera at a QR code"
			: "No QR code found in this image";
		ofDrawBitmapString(hint, previewX + 20, previewY + previewH - 15);
	}
}

void ofApp::drawResultBanner() {
	float bannerY = previewY + previewH + 15;
	float bannerH = 55;

	switch (bannerState) {
	case BannerState::EMPTY: {
		ofSetColor(240, 240, 235);
		ofDrawRectRounded(previewX, bannerY, previewW, bannerH, 8);
		ofSetColor(120);
		ofDrawBitmapString("No code scanned yet", previewX + 15, bannerY + 25);
		ofSetColor(160);
		ofDrawBitmapString("Results will appear here", previewX + 15, bannerY + 42);
		break;
	}

	case BannerState::PENDING: {
		ofSetColor(235, 235, 225);
		ofDrawRectRounded(previewX, bannerY, previewW, bannerH, 8);
		ofSetColor(90);
		ofDrawBitmapString("Scanned - safety check pending", previewX + 15, bannerY + 25);

		// "Check safety" button, right-aligned inside the banner.
		checkButtonBounds = ofRectangle(previewX + previewW - 130, bannerY + 12, 115, 30);

		// The URL sits on the row below the button, but the button is
		// wide enough to overlap that row too - so its available width
		// has to stop before the button starts, not run the full banner
		// width, or a long URL renders underneath it.
		float urlGap = 10;
		float urlMaxWidth = checkButtonBounds.x - (previewX + 15) - urlGap;
		ofSetColor(120);
		ofDrawBitmapString(truncateToFit(scanner.getLastDecoded(), urlMaxWidth), previewX + 15, bannerY + 42);

		ofSetColor(29, 158, 117);
		ofDrawRectRounded(checkButtonBounds, 6);
		ofSetColor(255);
		ofDrawBitmapString("Check safety", checkButtonBounds.x + 10, checkButtonBounds.y + 19);
		break;
	}

	case BannerState::NOT_A_LINK: {
		ofSetColor(235, 235, 235);
		ofDrawRectRounded(previewX, bannerY, previewW, bannerH, 8);
		ofSetColor(90);
		ofDrawBitmapString("Decoded text (not a link)", previewX + 15, bannerY + 22);
		ofSetColor(120);
		ofDrawBitmapString(truncateToFit(scanner.getLastDecoded(), previewW - 30), previewX + 15, bannerY + 42);
		// No safety check offered - there's no URL to check against
		// Safe Browsing, and offering one would imply this text was
		// actually verified when it wasn't.
		break;
	}

	case BannerState::SAFE: {
		ofSetColor(224, 245, 235);
		ofDrawRectRounded(previewX, bannerY, previewW, bannerH, 8);
		ofSetColor(29, 158, 117);
		ofDrawBitmapString("Safe link", previewX + 15, bannerY + 22);

		urlTextBounds = ofRectangle(previewX + 15, bannerY + 28, previewW - 30, 18);
		std::string url = truncateToFit(scanner.getLastDecoded(), urlTextBounds.width);
		ofSetColor(20, 110, 85);
		ofDrawBitmapString(url, urlTextBounds.x, urlTextBounds.y + 12);
		// Underline hint that this text is clickable (approximate width -
		// OF's default bitmap font is ~8px/char).
		ofDrawLine(urlTextBounds.x, urlTextBounds.y + 14,
			urlTextBounds.x + url.size() * 8.0f, urlTextBounds.y + 14);
		break;
	}

	case BannerState::UNSAFE: {
		ofSetColor(250, 226, 224);
		ofDrawRectRounded(previewX, bannerY, previewW, bannerH, 8);
		ofSetColor(178, 40, 36);
		ofDrawBitmapString("Unsafe link - do not open", previewX + 15, bannerY + 22);

		urlTextBounds = ofRectangle(previewX + 15, bannerY + 28, previewW - 30, 18);
		ofSetColor(178, 40, 36);
		ofDrawBitmapString(truncateToFit(scanner.getLastDecoded(), urlTextBounds.width),
			urlTextBounds.x, urlTextBounds.y + 12);
		break;
	}

	case BannerState::COULD_NOT_VERIFY: {
		ofSetColor(238, 236, 224);
		ofDrawRectRounded(previewX, bannerY, previewW, bannerH, 8);
		ofSetColor(140, 120, 40);
		ofDrawBitmapString("Couldn't verify link safety", previewX + 15, bannerY + 22);
		ofSetColor(120, 105, 60);
		ofDrawBitmapString(truncateToFit(scanner.getLastDecoded(), previewW - 30), previewX + 15, bannerY + 42);
		// Intentionally not clickable - see handleUrlClick().
		break;
	}
	}
}

void ofApp::drawConfirmDialog() {
	// Dim the whole window behind the dialog.
	ofSetColor(0, 0, 0, 150);
	ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());

	// Sized to leave a margin inside the 460px main window rather than
	// matching it edge to edge.
	float dialogW = 400, dialogH = 150;
	float dialogX = (ofGetWidth() - dialogW) / 2;
	float dialogY = (ofGetHeight() - dialogH) / 2;
	float dialogTextW = dialogW - 40; // inner margin, 20px each side

	ofSetColor(255);
	ofDrawRectRounded(dialogX, dialogY, dialogW, dialogH, 10);

	ofSetColor(30);
	ofDrawBitmapString("Open this link anyway?", dialogX + 20, dialogY + 30);
	ofSetColor(90);
	ofDrawBitmapString(truncateToFit(scanner.getLastDecoded(), dialogTextW), dialogX + 20, dialogY + 55);
	ofSetColor(178, 40, 36);
	ofDrawBitmapString("Flagged as unsafe by Google Safe Browsing", dialogX + 20, dialogY + 75);

	// Cancel = default/low-risk action, on the left. "Open anyway" = the
	// deliberate override, visually distinct in red on the right.
	confirmCancelBounds = ofRectangle(dialogX + 20, dialogY + dialogH - 45, 130, 30);
	confirmOpenBounds = ofRectangle(dialogX + dialogW - 150, dialogY + dialogH - 45, 130, 30);

	ofSetColor(230);
	ofDrawRectRounded(confirmCancelBounds, 6);
	ofSetColor(70);
	ofDrawBitmapString("Cancel", confirmCancelBounds.x + 35, confirmCancelBounds.y + 19);

	ofSetColor(178, 40, 36);
	ofDrawRectRounded(confirmOpenBounds, 6);
	ofSetColor(255);
	ofDrawBitmapString("Open anyway", confirmOpenBounds.x + 12, confirmOpenBounds.y + 19);
}

void ofApp::drawErrorPopup() {
	// Same dimmed-backdrop + centered box language as the confirm dialog,
	// for visual consistency - just a single acknowledgement button.
	ofSetColor(0, 0, 0, 150);
	ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());

	float dialogW = 400, dialogH = 130;
	float dialogX = (ofGetWidth() - dialogW) / 2;
	float dialogY = (ofGetHeight() - dialogH) / 2;

	ofSetColor(255);
	ofDrawRectRounded(dialogX, dialogY, dialogW, dialogH, 10);

	ofSetColor(30);
	// Messages are kept short and specific at each call site so they fit on
	// one line here, rather than adding wrap logic for what should be a
	// brief, glanceable notice.
	ofDrawBitmapString(errorPopupMessage, dialogX + 20, dialogY + 40);

	errorPopupOkBounds = ofRectangle(dialogX + (dialogW - 100) / 2, dialogY + dialogH - 45, 100, 30);
	ofSetColor(29, 158, 117);
	ofDrawRectRounded(errorPopupOkBounds, 6);
	ofSetColor(255);
	ofDrawBitmapString("OK", errorPopupOkBounds.x + 40, errorPopupOkBounds.y + 19);
}

void ofApp::checkSafetyNow() {
	LinkSafetyChecker checker(apiKey);
	SafetyResult result = checker.check(scanner.getLastDecoded());

	switch (result) {
	case SafetyResult::SAFE:
		bannerState = BannerState::SAFE;
		break;
	case SafetyResult::UNSAFE:
		bannerState = BannerState::UNSAFE;
		break;
	case SafetyResult::COULD_NOT_VERIFY:
		bannerState = BannerState::COULD_NOT_VERIFY;
		break;
	}
}

void ofApp::handleUrlClick() {
	if (bannerState == BannerState::SAFE) {
		openInBrowser(scanner.getLastDecoded());
	} else if (bannerState == BannerState::UNSAFE) {
		showConfirmDialog = true;
	}
	// COULD_NOT_VERIFY has no click behaviour by design - we can't vouch
	// for the link either way, so it stays inert rather than guessing.
}

void ofApp::mousePressed(int x, int y, int button) {
	if (showErrorPopup) {
		if (errorPopupOkBounds.inside(x, y)) {
			showErrorPopup = false;
		}
		return; // modal - ignore everything else while an error is showing
	}

	if (showConfirmDialog) {
		if (confirmOpenBounds.inside(x, y)) {
			openInBrowser(scanner.getLastDecoded());
			showConfirmDialog = false;
		} else if (confirmCancelBounds.inside(x, y)) {
			showConfirmDialog = false;
		}
		return; // modal - ignore everything else in the app while open
	}

	for (const auto & btn : toggleButtons) {
		if (btn.bounds.inside(x, y)) {
			if (btn.isCameraButton) {
				if (scanner.isCameraAvailable()) {
					isCameraMode = true;
					scanner.setModeCamera();
				} else {
					showError("No camera detected. Use File mode instead.");
				}
			} else {
				ofFileDialogResult result = ofSystemLoadDialog("Select a QR code image");
				if (result.bSuccess) {
					if (scanner.loadImageFile(result.getPath())) {
						isCameraMode = false;
						if (!scanner.hasDecoded()) {
							showError("No QR code found in that image.");
						}
					} else {
						showError("Couldn't load that image. Try another file.");
					}
				}
			}
			return;
		}
	}

	if (bannerState == BannerState::PENDING && checkButtonBounds.inside(x, y)) {
		checkSafetyNow();
		return;
	}

	if ((bannerState == BannerState::SAFE || bannerState == BannerState::UNSAFE)
		&& urlTextBounds.inside(x, y)) {
		handleUrlClick();
		return;
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
