#include "QrDecoder.h"
#include <cstring>

QrDecoder::QrDecoder() {
	qr = quirc_new();
}

QrDecoder::~QrDecoder() {
	if (qr) {
		quirc_destroy(qr);
	}
}

std::string QrDecoder::decode(const ofPixels & pixels) {
	if (!qr) return "";

	int w = pixels.getWidth();
	int h = pixels.getHeight();

	// Resize quirc's internal buffer only when the incoming image size actually changes
	if (w != currentWidth || h != currentHeight) {
		if (quirc_resize(qr, w, h) < 0) {
			return "";
		}
		currentWidth = w;
		currentHeight = h;
	}

	ofPixels gray = pixels;
	gray.setImageType(OF_IMAGE_GRAYSCALE);

	int qw, qh;
	uint8_t * buffer = quirc_begin(qr, &qw, &qh);
	memcpy(buffer, gray.getData(), static_cast<size_t>(qw) * qh);
	quirc_end(qr);

	int count = quirc_count(qr);
	for (int i = 0; i < count; i++) {
		struct quirc_code code;
		struct quirc_data data;
		quirc_extract(qr, i, &code);

		if (quirc_decode(&code, &data) == QUIRC_SUCCESS) {
			return std::string(reinterpret_cast<char *>(data.payload));
		}
	}
	return "";
}
