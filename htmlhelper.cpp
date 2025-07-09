#include "htmlhelper.h"

String minifyHTML(String html) {
  html.replace("\n", "");
  html.replace("\r", "");
  html.replace("\t", "");
  while (html.indexOf("> <") != -1) {
    html.replace("> <", "><");
  }
  while (html.indexOf("  ") != -1) {
    html.replace("  ", " ");
  }
  int eqPos = html.indexOf(" =");
  while (eqPos != -1) {
    html.remove(eqPos, 1);
    eqPos = html.indexOf(" =", eqPos);
  }

  eqPos = html.indexOf("= ");
  while (eqPos != -1) {
    html.remove(eqPos + 1, 1);
    eqPos = html.indexOf("= ", eqPos);
  }
  html.trim();
  return html;
}

String fun(String html) {
  String comment = "<!-- \n"
                   "  Hey there, code explorer!\n"
                   "  You won't find secrets here... or will you?\n"
                   "  https://github.com/daongochuy2516/ESP32WakeOnLanSinricPro\n"
                   "-->\n";
  return comment + html;
}