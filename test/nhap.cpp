StaticJsonDocument<200> parseClientJson(const String& msg) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, msg);
  StaticJsonDocument<200> result;
  if (!error) {
    result["role"] = doc["role"] | "";
    result["status"] = doc["status"] | "";
    result["data"] = doc["data"] | "";
  } else {
    result["role"] = "";
    result["status"] = "";
    result["data"] = "";
  }
  return result;
}
