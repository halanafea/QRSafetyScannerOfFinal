#include "ApiKeyLoader.h"

std::string ApiKeyLoader::load(const std::string & filename) {
	ofFile file(filename);
	if (!file.exists()) {
		ofLogWarning() << "API key file not found: " << filename;
		return "";
	}

	ofBuffer buffer = ofBufferFromFile(filename);
	std::string key = buffer.getText();

	// Trim any trailing whitespace/newline from the file
	while (!key.empty() && (key.back() == '\n' || key.back() == '\r' || key.back() == ' ')) {
		key.pop_back();
	}

	return key;
}
