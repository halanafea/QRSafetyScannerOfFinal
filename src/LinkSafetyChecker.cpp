#include "LinkSafetyChecker.h"
#include "json.hpp"
#include <curl/curl.h>

// ============================================================
// TEMPORARY - LOCAL DEV ONLY. Remove this line before final
// submission/commit. Disables SSL verification to work around
// this machine's antivirus HTTPS scanning. Must never ship.
// ============================================================
#define QR_SCANNER_DISABLE_SSL_VERIFY_FOR_LOCAL_DEV

using json = nlohmann::json;

LinkSafetyChecker::LinkSafetyChecker(const std::string & apiKey)
	: apiKey(apiKey) { }

size_t LinkSafetyChecker::writeCallback(void * contents, size_t size, size_t nmemb, std::string * output) {
	size_t totalSize = size * nmemb;
	output->append(static_cast<char *>(contents), totalSize);
	return totalSize;
}

SafetyResult LinkSafetyChecker::check(const std::string & url) {
	if (apiKey.empty()) {
		ofLogWarning() << "No API key available - cannot check link safety";
		return SafetyResult::COULD_NOT_VERIFY;
	}

	CURL * curl = curl_easy_init();
	if (!curl) {
		return SafetyResult::COULD_NOT_VERIFY;
	}

	json requestBody = {
		{ "client", { { "clientId", "qr-safety-scanner" }, { "clientVersion", "1.0" } } },
		{ "threatInfo", { { "threatTypes", { "MALWARE", "SOCIAL_ENGINEERING", "UNWANTED_SOFTWARE", "POTENTIALLY_HARMFUL_APPLICATION" } }, { "platformTypes", { "ANY_PLATFORM" } }, { "threatEntryTypes", { "URL" } }, { "threatEntries", { { { "url", url } } } } } }
	};
	std::string requestBodyStr = requestBody.dump();

	std::string endpoint = "https://safebrowsing.googleapis.com/v4/threatMatches:find?key=" + apiKey;
	std::string response;

	struct curl_slist * headers = nullptr;
	headers = curl_slist_append(headers, "Content-Type: application/json");

	curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBodyStr.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
	curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NO_REVOKE);

#ifdef QR_SCANNER_DISABLE_SSL_VERIFY_FOR_LOCAL_DEV
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

	CURLcode res = curl_easy_perform(curl);

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	if (res != CURLE_OK) {
		ofLogError() << "Safe Browsing API request failed: " << curl_easy_strerror(res);
		return SafetyResult::COULD_NOT_VERIFY;
	}

	try {
		json responseJson = json::parse(response);
		if (responseJson.contains("matches") && !responseJson["matches"].empty()) {
			return SafetyResult::UNSAFE;
		}
		return SafetyResult::SAFE;
	} catch (const json::parse_error & e) {
		ofLogError() << "Failed to parse Safe Browsing API response: " << e.what();
		return SafetyResult::COULD_NOT_VERIFY;
	}
}
