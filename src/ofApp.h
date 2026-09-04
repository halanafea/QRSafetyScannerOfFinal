#pragma once
#include "Scanner.h"
#include "ofMain.h"

struct ToggleButton {
	std::string label;
	ofRectangle bounds;
	bool isCameraButton;
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

	Scanner scanner;
	std::vector<ToggleButton> toggleButtons;

	float previewX = 20, previewY = 60, previewW = 420, previewH = 315;
	bool isCameraMode = true;
};
