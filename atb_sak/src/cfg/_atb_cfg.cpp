#include <fstream>
#include <sstream>
#include "_atb_cfg.h"

int get_config_map(const char *config_file, std::map<std::string, std::string> &configs) {
    std::ifstream fs(config_file);
    if(!fs.is_open()) {
        return -1;
    }

    while(fs.good()) {
        std::string line;
        std::getline(fs, line);

        if (line[0] == '#') {
            continue;
        }
        std::stringstream ss;
        ss << line;
        std::string key, value;
        std::getline(ss, key, '=');
        std::getline(ss, value, '=');

        configs[key] = value;
    }
    fs.close();
    return 0;
}

