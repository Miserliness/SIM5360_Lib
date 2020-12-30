#include <stdlib.h>
//#include "diodes.h"
#include "Arduino.h"

#include <cJSON.h>

extern int httpRequest(String method, String host, String url, String header, String data, String *response);
// extern void ApiBoxAuth();
extern String get_body(String buf, size_t beg);
// extern void postMeasures(int GLOBAL_FRAMEWAREVERSION, int LOCAL_FRAMEWAREVERSION);
extern void load_wifi_data_from_memory();
// extern void getNewToken();
extern void jsonparseTOKEN(String str);
extern int errPostMesures(String str);