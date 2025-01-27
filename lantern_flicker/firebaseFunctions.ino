#include "firebaseFunctions.h"

//#define WLD_DEBUG
void writeLocalData()
{
  if (!SPIFFS.format()) {
#ifdef WLD_DEBUG
    Serial.println("File System Formatting Failed.");
#endif
    return;
  }

  File f = SPIFFS.open(LanternFilePath, "w");
  if (!f) {
#ifdef WLD_DEBUG
    Serial.println("Failed to open file.");
#endif
    return;
  }

  f.write(((uint8_t *) &Lantern), sizeof(LanternData));
  f.flush();
  f.close();

#ifdef WLD_DEBUG
  Serial.println("Finished writing Lantern to local storage.");
#endif
}


//#define CON_WIFI_DEBUG
void connectToWiFi() {
  WiFiSetup = true;
  WiFi.begin(WIFI_AP_NAME, WIFI_AP_PASS);
}

void connectToFirebase() {
  FirebaseSetup = true;
#ifdef CON_WIFI_DEBUG
	Serial.println();
	Serial.print("Connected with IP: ");
	Serial.println(WiFi.localIP());
	Serial.println();
#endif // CON_WIFI_DEBUG

    // Connect to Firebase
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    Firebase.reconnectWiFi(true);

    if (!Firebase.beginStream(firebaseDataRECV, DevicePath)) {
#ifdef CON_WIFI_DEBUG
        Serial.println("------------------------------------");
        Serial.println("Can't begin stream connection...");
        Serial.println("REASON: " + firebaseDataRECV.errorReason());
        Serial.println("------------------------------------");
        Serial.println();
#endif
    }

    Firebase.setStreamCallback(firebaseDataRECV, handleDataRecieved, handleTimeout);
}

//#define SETUP_FF_DEBUG
void setupFirebaseFunctions() {
    char* temp = (char* )malloc(50 * sizeof(char));

	sprintf(temp, "/devices/%lu", ESP.getChipId());
    DevicePath = String(temp);

    delete[] temp;

#ifdef SETUP_FF_DEBUG
    Serial.print("DevicePath: ");
    Serial.println(DevicePath);
#endif
}

//#define HAR_DEBUG
void handleDataRecieved(StreamData data) {
    if (data.dataType() == "json") {
#ifdef HAR_DEBUG
        Serial.println("Stream data available...");
        Serial.println("STREAM PATH: " + data.streamPath());
        Serial.println("EVENT PATH: " + data.dataPath());
        Serial.println("DATA TYPE: " + data.dataType());
        Serial.println("EVENT TYPE: " + data.eventType());
        Serial.print("VALUE: ");
        printResult(data);
        Serial.println();
#endif

        FirebaseJson *json = data.jsonObjectPtr();
        size_t len = json->iteratorBegin();
        String key, value = "";
        int type = 0;
        int temp = -1;

        for (size_t i = 0; i < len; i++) {
            json->iteratorGet(i, type, key, value);
            if (key == "pin")
                Lantern.pin = value.toInt();
            else if (key == "maxBrightness")
                Lantern.maxBrightness = value.toInt();
            else if (key == "minBrightness")
                Lantern.minBrightness = value.toInt();
            else if (key == "smoothing") {
              temp = value.toInt();
              if (temp > 0) Lantern.smoothing = temp;
            }
            else if (key == "rampDelay") {
              temp = value.toInt();
              if (temp > 0) Lantern.rampDelay = value.toInt();
            }
            else if (key == "dropDelay") {
              temp = value.toInt();
              if (temp > 0) Lantern.dropDelay = value.toInt();
            }
            else if (key == "dropValue") {
                temp = value.toInt();
                if (temp > 0) Lantern.dropValue = value.toInt();
            }
            else if (key == "flickerDelayMin") {
                temp = value.toInt();
                if (temp > 0) Lantern.flickerDelayMin = value.toInt();
            }
            else if (key == "flickerDelayMax") {
                temp = value.toInt();
                if (temp > 0) Lantern.flickerDelayMax = value.toInt();
            }
            else {
#ifdef HAR_DEBUG
        Serial.println("Skipping value...");
        Serial.print("TYPE: ");
        Serial.println(type == FirebaseJson::JSON_OBJECT ? "object" : "array");
        Serial.print("KEY: ");
        Serial.println(key);
#endif                
            }
            newDataReceived = true;
        }
        writeLocalData();
    } else if (data.dataType() == "null") {
#ifdef HAR_DEBUG
      Serial.println("No endpoint found on backend. Creating new device...");
#endif
      FirebaseJson json;
      json.add("dropDelay", Lantern.dropDelay);
      json.add("dropValue", Lantern.dropValue);
      json.add("rampDelay", Lantern.rampDelay);
      json.add("groupName", "Lanterns");
      json.add("name", String(ESP.getChipId()));
      json.add("maxBrightness", Lantern.maxBrightness);
      json.add("minBrightness", Lantern.minBrightness);
      json.add("smoothing", Lantern.smoothing);
      json.add("pin", Lantern.pin);
      json.add("flickerDelayMin", Lantern.flickerDelayMin);
      json.add("flickerDelayMax", Lantern.flickerDelayMax);
      Firebase.set(firebaseDataSEND, DevicePath, json);
      delay(3000);
    } else {
#ifdef HAR_DEBUG
    Serial.print("Stream returned non-JSON response: ");
    Serial.println(data.dataType());
#endif
    }
}

void handleTimeout(bool timeout) {
#ifdef TIMEOUT_DEBUG
  if (timeout)
  {
    Serial.println();
    Serial.println("Stream timeout, resume streaming...");
    Serial.println();
  }
#endif
}


/** -------- DEBUG FUNCTION -------- **/

#if defined(HAR_DEBUG)
void printResult(StreamData &data)
{

  if (data.dataType() == "int")
    Serial.println(data.intData());
  else if (data.dataType() == "float")
    Serial.println(data.floatData(), 5);
  else if (data.dataType() == "double")
    printf("%.9lf\n", data.doubleData());
  else if (data.dataType() == "boolean")
    Serial.println(data.boolData() == 1 ? "true" : "false");
  else if (data.dataType() == "string" || data.dataType() == "null")
    Serial.println(data.stringData());
  else if (data.dataType() == "json")
  {
    Serial.println();
    FirebaseJson *json = data.jsonObjectPtr();
    //Print all object data
    Serial.println("Pretty printed JSON data:");
    String jsonStr;
    json->toString(jsonStr, true);
    Serial.println(jsonStr);
    Serial.println();
    Serial.println("Iterate JSON data:");
    Serial.println();
    size_t len = json->iteratorBegin();
    String key, value = "";
    int type = 0;
    for (size_t i = 0; i < len; i++)
    {
      json->iteratorGet(i, type, key, value);
      Serial.print(i);
      Serial.print(", ");
      Serial.print("Type: ");
      Serial.print(type == FirebaseJson::JSON_OBJECT ? "object" : "array");
      if (type == FirebaseJson::JSON_OBJECT)
      {
        Serial.print(", Key: ");
        Serial.print(key);
      }
      Serial.print(", Value: ");
      Serial.println(value);
    }
    json->iteratorEnd();
  }
  else if (data.dataType() == "array")
  {
    Serial.println();
    //get array data from FirebaseData using FirebaseJsonArray object
    FirebaseJsonArray *arr = data.jsonArrayPtr();
    //Print all array values
    Serial.println("Pretty printed Array:");
    String arrStr;
    arr->toString(arrStr, true);
    Serial.println(arrStr);
    Serial.println();
    Serial.println("Iterate array values:");
    Serial.println();

    for (size_t i = 0; i < arr->size(); i++)
    {
      Serial.print(i);
      Serial.print(", Value: ");

      FirebaseJsonData *jsonData = data.jsonDataPtr();
      //Get the result data from FirebaseJsonArray object
      arr->get(*jsonData, i);
      if (jsonData->typeNum == FirebaseJson::JSON_BOOL)
        Serial.println(jsonData->boolValue ? "true" : "false");
      else if (jsonData->typeNum == FirebaseJson::JSON_INT)
        Serial.println(jsonData->intValue);
      else if (jsonData->typeNum == FirebaseJson::JSON_FLOAT)
        Serial.println(jsonData->floatValue);
      else if (jsonData->typeNum == FirebaseJson::JSON_DOUBLE)
        printf("%.9lf\n", jsonData->doubleValue);
      else if (jsonData->typeNum == FirebaseJson::JSON_STRING ||
               jsonData->typeNum == FirebaseJson::JSON_NULL ||
               jsonData->typeNum == FirebaseJson::JSON_OBJECT ||
               jsonData->typeNum == FirebaseJson::JSON_ARRAY)
        Serial.println(jsonData->stringValue);
    }
  }
  else if (data.dataType() == "blob")
  {

    Serial.println();

    for (int i = 0; i < data.blobData().size(); i++)
    {
      if (i > 0 && i % 16 == 0)
        Serial.println();

      if (i < 16)
        Serial.print("0");

      Serial.print(data.blobData()[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
  else if (data.dataType() == "file")
  {

    Serial.println();

    File file = data.fileStream();
    int i = 0;

    while (file.available())
    {
      if (i > 0 && i % 16 == 0)
        Serial.println();

      int v = file.read();

      if (v < 16)
        Serial.print("0");

      Serial.print(v, HEX);
      Serial.print(" ");
      i++;
    }
    Serial.println();
    file.close();
  }
}
#endif

