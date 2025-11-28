#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include "nlohmann/json.hpp"

using namespace std;
using Json = nlohmann::json;

std::string ReplaceAll(std::string str, const std::string &src, const std::string &dst) {
    std::string::size_type pos(0);
    int diff = dst.length() - src.length();
    diff = diff > 0 ? diff + 1 : 1;
    while ((pos = str.find(src, pos)) != std::string::npos) {
        str.replace(pos, src.length(), dst);
        pos += diff;
    }
    return str;
}

int main() {
    ifstream ifs2("settings.ini");

    if (ifs2.good()) {
        ifstream ifs;
        ifs.open("settings.ini", ios::in);

        string buff, jsonStr;
        while (getline(ifs, buff)) {
            jsonStr.append(buff);
        }

        ifs.close();

        jsonStr = ReplaceAll(jsonStr, "\\", "\\\\");

        Json jsonData = Json::parse(jsonStr);

        for (Json &mouse : jsonData["mouse"]) {
            for (Json &handle : mouse["handles"]) {
                if (handle["duration"] > 0) {
                    vector<unsigned long long> v;
                    for (int i = 0; i < handle["btnDown"].size(); i++) {
                        v.push_back(0);
                    }
                    handle["dTime"] = Json(v);
                    handle["uTime"] = Json(v);
                }
            }
        }

        cout << jsonData.dump() << endl;
    }
}