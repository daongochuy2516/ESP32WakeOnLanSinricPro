#pragma once

#include <Arduino.h>

class Version {
  public:
    Version(const String& versionStr);
    String toString() const;

    bool operator > (const Version& other) const;
    bool operator < (const Version& other) const;
    bool operator == (const Version& other) const;
    bool operator != (const Version& other) const;

  protected:
    int major;
    int minor;
    int patch;
};
