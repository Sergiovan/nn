#include "common/filesystem_include.h"
#include "reader.h"

namespace fs = std::filesystem;

reader::reader(std::string str, bool is_file) {
    if(is_file) {
        bool exists;
        try {
            exists = fs::exists(str);
        } catch(...) {
            exists = false;
        }
        if(exists) {
        
        } else {
            std::printf("File %s does not exist!\n", str.c_str());
        }
    } else {
    
    }
}
