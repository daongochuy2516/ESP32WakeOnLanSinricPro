#include "SemVer.h"

Version::Version(const String& versionStr) {
    int     firstDot  = versionStr.indexOf('.');
    int     secondDot = versionStr.lastIndexOf('.');
    major           = versionStr.substring(0, firstDot).toInt();
    minor           = versionStr.substring(firstDot + 1, secondDot).toInt();
    patch           = versionStr.substring(secondDot + 1).toInt();
}

String Version::toString() const {
    return String(major) + "." + String(minor) + "." + String(patch);
}

bool Version::operator>(const Version& other) const {
    if (major != other.major) return major > other.major;
    if (minor != other.minor) return minor > other.minor;
    return patch > other.patch;
}

bool Version::operator<(const Version& other) const {
    return other > *this;
}

bool Version::operator==(const Version& other) const {
    if (major != other.major) return false;
    if (minor != other.minor) return false;
    if (patch != other.patch) return false;
    return true;
}

bool Version::operator!=(const Version& other) const {
    return !operator==(other);
}
