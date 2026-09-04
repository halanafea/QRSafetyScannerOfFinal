#pragma once
#include "LinkSafetyChecker.h"
#include "Scanner.h"
#include "ofMain.h"

struct ToggleButton {
	std::string label;
	ofRectangle bounds;
	bool isCameraButton;
};

// Result-banner state. EMPTY/PENDING are scan-flow states; the other three
// mirror SafetyResult 1:1 once a check has actually been run.
enum class BannerState {
	EMPTY, // nothing decoded yet
	PENDING, // decoded, safety check not run yet
	SAFE,
	UNSAFE,
	COULD_NOT_VERIFY
};

class ofApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void mouseScrolled(int x, int y, float scrollX, float scrollY);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

private:
	void drawHeader();
	void drawPreview();
	void drawResultBanner();
	void drawConfirmDialog();

	void checkSafetyNow();
	void handleUrlClick();
	std::string truncateToFit(const std::string & text, float maxWidth) const;

	Scanner scanner;
	std::vector<ToggleButton> toggleButtons;

	float previewX = 20, previewY = 60, previewW = 420, previewH = 315;
	bool isCameraMode = true;

	// --- Phase 6: safety verdict wiring ---
	std::string apiKey;
	std::string lastSeenDecodedUrl; // used to detect a newly-decoded code
	BannerState bannerState = BannerState::EMPTY;

	ofRectangle checkButtonBounds; // "Check safety" button, shown while PENDING
	ofRectangle urlTextBounds; // decoded URL text, clickable when SAFE/UNSAFE

	bool showConfirmDialog = false;
	ofRectangle confirmCancelBounds;
	ofRectangle confirmOpenBounds;
};
