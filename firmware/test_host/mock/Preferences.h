#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

class Preferences {
  std::map<std::string, double> kv;
  std::string path;
  void save() {
    if (path.empty()) return;
    FILE *f = fopen(path.c_str(), "w");
    for (auto &p : kv) fprintf(f, "%s %.9f\n", p.first.c_str(), p.second);
    fclose(f);
  }

 public:
  int writes = 0;
  bool begin(const char *, bool) {
    const char *p = getenv("SIM_PREFS");
    path = p ? p : "";
    if (FILE *f = path.empty() ? nullptr : fopen(path.c_str(), "r")) {
      char k[64];
      double v;
      while (fscanf(f, "%63s %lf", k, &v) == 2) kv[k] = v;
      fclose(f);
    }
    return true;
  }
  double getDouble(const char *k, double def) { return kv.count(k) ? kv[k] : def; }
  size_t putDouble(const char *k, double v) {
    kv[k] = v;
    writes++;
    save();
    return 8;
  }
};
