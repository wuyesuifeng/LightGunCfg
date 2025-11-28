#include <string>
#include <iostream>

using namespace std;

int findLastOfIgnoreCase(string str, string dst) {
    static int i, j, dstLen;
    static char dstLast;
    dstLen = dst.length() - 1;
    dstLast = toupper(dst.at(dstLen));
    for (i = str.length() - 1; i >= dstLen; i--) {
        if (toupper(str.at(i)) == dstLast) {
            for (j = 1; j < dstLen; j++) {
                if (toupper(str.at(i - j)) != toupper(dst.at(dstLen - j))) {
                    goto for1;
                }
            }
            return i + 1;
        }
    for1:
        continue;
    }
    return -1;
}

int main() {
    string str("L:\\ISO\\TeknoParrot\\LaunchBox\\LazyLauncher\\Items\\ringwide_og_cn\\gs2.exe");
    int res = findLastOf(str, "\\Ringwide_Og_cn\\gS2.exe");
    cout << "res: " << (res == str.length()) << endl;
}