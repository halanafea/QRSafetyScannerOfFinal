#pragma once
#include "ofMain.h"

class ApiKeyLoader {
public:
	// Returns the key, or empty string if the file is missing/empty
	static std::string load(const std::string & filename);
};
