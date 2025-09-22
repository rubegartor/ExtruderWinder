#pragma once

#include <Arduino.h>
#include "KVStore.h"
#include "kvstore_global_api.h"

class Storage {
  public:
    String getString(const char* key, const char* defaultValue);
    int getInt(const char* key, int defaultValue);
    float getFloat(const char* key, float defaultValue);
    bool getBool(const char* key, bool defaultValue);

    void setString(const char* key, const char* value);
    void setInt(const char* key, int value);
    void setFloat(const char* key, float value);
    void setBool(const char* key, bool value);

    void remove(const char* key);
};