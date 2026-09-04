#pragma once
#include "ofMain.h"

enum class SafetyResult { SAFE,
	UNSAFE,
	COULD_NOT_VERIFY };

class LinkSafetyChecker {
public:
	explicit LinkSafetyChecker(const std::string & apiKey);
	SafetyResult check(const std::string & url);

private:
	static size_t writeCallback(void * contents, size_t size, size_t nmemb, std::string * output);
	std::string apiKey;
};
