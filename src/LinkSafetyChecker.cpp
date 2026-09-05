#include "LinkSafetyChecker.h"
#include "json.hpp"
#include <curl/curl.h>

namespace {
int curlDebugCallback(CURL * handle, curl_infotype type, char * data, size_t size, void * userptr) {
	if (type == CURLINFO_TEXT) {
		std::string msg(data, size);
		ofLogNotice() << "[curl] " << msg;
	}
	return 0;
}
}

// ============================================================
// TEMPORARY - LOCAL DEV ONLY. Remove this line before final
// submission/commit. Disables SSL verification to work around
// this machine's antivirus HTTPS scanning. Must never ship.
// See documentation Section 13.2 / 13.3 and the Phase 9
// pre-submission checklist.
// ============================================================
// #define QR_SCANNER_DISABLE_SSL_VERIFY_FOR_LOCAL_DEV

using json = nlohmann::json;

LinkSafetyChecker::LinkSafetyChecker(const std::string & apiKey)
	: apiKey(apiKey) { }

size_t LinkSafetyChecker::writeCallback(void * contents, size_t size, size_t nmemb, std::string * output) {
	size_t totalSize = size * nmemb;
	output->append(static_cast<char *>(contents), totalSize);
	return totalSize;
}

SafetyResult LinkSafetyChecker::check(const std::string & url) {
	// TEMPORARY DIAGNOSTIC - remove after checking the console output once.
	// Confirms which TLS backend curl is actually using at runtime, rather
	// than assuming based on the package's feature-definition file.
	ofLogNotice() << "curl version info: " << curl_version();

	if (apiKey.empty()) {
		ofLogWarning() << "No API key available - cannot check link safety";
		return SafetyResult::COULD_NOT_VERIFY;
	}

	CURL * curl = curl_easy_init();
	if (!curl) {
		return SafetyResult::COULD_NOT_VERIFY;
	}

	// Build the request body per Google Safe Browsing API v4 spec
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
	curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NO_REVOKE | CURLSSLOPT_NATIVE_CA);
	// openFrameworks bundles its own OpenSSL-linked curl (confirmed during
	// Phase 9 investigation - see documentation Section 16). CURLSSLOPT_NATIVE_CA
	// above is a no-op on some OpenSSL builds that lack Windows cert-store
	// bridging support compiled in, so it isn't relied on alone: pointing
	// CAINFO at a bundled CA file works on any OpenSSL build regardless of
	// platform, so this is the fix actually being relied on.
	std::string caBundlePath = ofToDataPath("cacert.pem", true);
	// TEMPORARY DIAGNOSTIC - remove once confirmed.
	ofLogNotice() << "CA bundle path: " << caBundlePath
				  << " (exists: " << (ofFile(caBundlePath).exists() ? "yes" : "NO") << ")";
	curl_easy_setopt(curl, CURLOPT_CAINFO, caBundlePath.c_str());
	// TEMPORARY DIAGNOSTIC - remove once resolved. Gets curl's actual
	// detailed handshake reasoning instead of just the generic top-level
	// error string.
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, curlDebugCallback);
	// Without these, a hung/unreachable network leaves check() blocking
	// indefinitely - and since it's called synchronously from a button
	// click, that freezes the whole app with no way out but force-quitting.
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L); // seconds to establish connection
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); // seconds for the whole request

#ifdef QR_SCANNER_DISABLE_SSL_VERIFY_FOR_LOCAL_DEV
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
#endif

	CURLcode res = curl_easy_perform(curl);

	long httpCode = 0;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);

	if (res != CURLE_OK) {
		ofLogError() << "Safe Browsing API request failed: " << curl_easy_strerror(res);
		return SafetyResult::COULD_NOT_VERIFY;
	}

	if (httpCode != 200) {
		// A non-200 response (bad/expired key, quota exceeded, server error,
		// etc.) may still return a parseable JSON body, but not one shaped
		// like a real threatMatches result - checking the status code first
		// avoids misreading an error body as "no matches found" == SAFE.
		ofLogError() << "Safe Browsing API returned HTTP " << httpCode << " - treating as unverifiable";
		return SafetyResult::COULD_NOT_VERIFY;
	}

	try {
		json responseJson = json::parse(response);
		if (responseJson.contains("matches") && responseJson["matches"].is_array()
			&& !responseJson["matches"].empty()) {
			return SafetyResult::UNSAFE;
		}
		return SafetyResult::SAFE; // empty {} response means no threats found
	} catch (const std::exception & e) {
		// Catches json::parse_error (invalid JSON) as well as json::type_error
		// and similar (valid JSON that doesn't match the shape we expect) -
		// either way, we can't trust the result, so don't guess.
		ofLogError() << "Failed to interpret Safe Browsing API response: " << e.what();
		return SafetyResult::COULD_NOT_VERIFY;
	}
}
