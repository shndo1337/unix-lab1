#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>

using namespace std;
using namespace std::filesystem;

string read_file(string path)
{
    ifstream file(path, ios::binary);
    string data;
    char c;
    while (file.get(c))
        data += c;
    return data;
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        cout << "Usage: ./main <directory>" << endl;
        return 1;
    }

    vector<string> paths;
    vector<size_t> hashes;
    hash<string> hasher;

    for (directory_entry entry : recursive_directory_iterator(argv[1])) {
        if (entry.is_regular_file()) {
            string path = entry.path().string();
            size_t h = hasher(read_file(path));

            bool found = false;
            for (int i = 0; i < hashes.size(); i++) {
                if (hashes[i] == h) {
                    remove(path);
                    create_hard_link(paths[i], path);
                    cout << path << " -> " << paths[i] << endl;
                    found = true;
                    break;
                }
            }

            if (!found) {
                paths.push_back(path);
                hashes.push_back(h);
            }
        }
    }
    return 0;
}
