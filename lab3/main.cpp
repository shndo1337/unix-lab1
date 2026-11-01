#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <openssl/sha.h>

using namespace std;
using namespace std::filesystem;

bool get_hash(string path, string& result)
{
    ifstream file(path, ios::binary);
    if (!file)
        return false;

    SHA_CTX ctx;
    SHA1_Init(&ctx);
    char buf[4096];
    while (file.read(buf, sizeof(buf)) || file.gcount() > 0)
        SHA1_Update(&ctx, buf, file.gcount());
    if (file.bad())
        return false;

    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1_Final(hash, &ctx);
    result = string((char*)hash, SHA_DIGEST_LENGTH);
    return true;
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        cout << "Usage: ./main <directory>" << endl;
        return 1;
    }

    vector<string> paths;
    vector<string> hashes;
    int total = 0, linked = 0, already = 0, errors = 0;

    try {
        for (directory_entry entry : recursive_directory_iterator(argv[1], directory_options::skip_permission_denied)) {
            if (entry.is_regular_file() && !entry.is_symlink()) {
                total++;
                string path = entry.path().string();
                string h;

                if (!get_hash(path, h)) {
                    cout << "Error: cannot read " << path << endl;
                    errors++;
                    continue;
                }

                bool found = false;
                for (int i = 0; i < hashes.size(); i++) {
                    if (hashes[i] == h) {
                        found = true;
                        error_code ec;

                        if (equivalent(paths[i], path, ec)) {
                            already++;
                        } else {
                            string tmp = path + ".tmp";
                            create_hard_link(paths[i], tmp, ec);
                            if (ec) {
                                cout << "Error: " << path << ": " << ec.message() << endl;
                                errors++;
                            } else {
                                rename(tmp, path, ec);
                                if (ec) {
                                    cout << "Error: " << path << ": " << ec.message() << endl;
                                    errors++;
                                    remove(tmp, ec);
                                } else {
                                    cout << path << " -> " << paths[i] << endl;
                                    linked++;
                                }
                            }
                        }
                        break;
                    }
                }

                if (!found) {
                    paths.push_back(path);
                    hashes.push_back(h);
                }
            }
        }
    } catch (filesystem_error& e) {
        cout << "Error: " << e.what() << endl;
        errors++;
    }

    cout << "Files: " << total << ", replaced with links: " << linked
         << ", already links: " << already << ", errors: " << errors << endl;
    return 0;
}
