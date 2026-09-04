#include "ofApp.h"
#include "ofMain.h"

//========================================================================
int main() {

	// Must be set before ANY ofLog*() call happens, anywhere in the app -
	// including calls made during setup() (e.g. ApiKeyLoader logging a
	// missing-file warning). Without this, the logger's internal channel
	// is a null shared_ptr and the first log call crashes with a read
	// access violation (see documentation Section 10.1).
	ofSetLoggerChannel(std::make_shared<ofConsoleLoggerChannel>());

	// ofGLFWWindowSettings (rather than the plain ofGLWindowSettings) is
	// used specifically so we can set resizable = false below.
	ofGLFWWindowSettings settings;
	settings.setSize(460, 480); // create at the app's real target size directly -
	// no runtime resize needed/wanted (see note below)
	settings.resizable = false; // locked single-panel layout; not designed to be resized
	settings.windowMode = OF_WINDOW; // can also be OF_FULLSCREEN

	auto window = ofCreateWindow(settings);

	ofRunApp(window, std::make_shared<ofApp>());
	ofRunMainLoop();
}
