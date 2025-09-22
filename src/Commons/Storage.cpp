#include "Storage.h"

String Storage::getString(const char* key, const char* defaultValue) {
  char buffer[256];
  size_t value_size;
  int result = kv_get(key, buffer, sizeof(buffer), &value_size);
  if (result == 0) {
    return String(buffer);
  }
  
  return String(defaultValue);
}

int Storage::getInt(const char* key, int defaultValue) {
  char buffer[256];
  size_t value_size;
  int result = kv_get(key, buffer, sizeof(buffer), &value_size);
  if (result == 0) {
    int value = atoi(buffer);
    return value;
  }
  
  return defaultValue;
}

float Storage::getFloat(const char* key, float defaultValue) {
  char buffer[256];
  size_t value_size;
  int result = kv_get(key, buffer, sizeof(buffer), &value_size);
  if (result == 0) {
    float value = atof(buffer);
    return value;
  }
  
  return defaultValue;
}

bool Storage::getBool(const char* key, bool defaultValue) {
  return this->getInt(key, defaultValue ? 1 : 0) == 1;
}

void Storage::setString(const char* key, const char* value) {
  kv_set(key, value, strlen(value) + 1, 0);
}

void Storage::setInt(const char* key, int value) {
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "%d", value);
  kv_set(key, buffer, strlen(buffer) + 1, 0);
}

void Storage::setFloat(const char* key, float value) {
  char buffer[256];
  snprintf(buffer, sizeof(buffer), "%.6f", value);
  kv_set(key, buffer, strlen(buffer) + 1, 0);
}

void Storage::setBool(const char* key, bool value) {
  this->setInt(key, value ? 1 : 0);
}

void Storage::remove(const char* key) {
  kv_remove(key);
}