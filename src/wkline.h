typedef struct WklineDimensions {
	int w;
	int h;
} WklineDimensions;

typedef struct WklineThreadData {
	WebKitWebView *web_view;
	char *buf;
} WklineThreadData;
