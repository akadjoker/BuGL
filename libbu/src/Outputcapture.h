#ifndef OUTPUTCAPTURE_H
#define OUTPUTCAPTURE_H

#include <string>

class OutputCapture {
public:
    std::string buffer;
    
    void write(const std::string& text) {
        buffer += text;
    }
    
    std::string getOutput() {
        return buffer;
    }
    
    void clear() {
        buffer.clear();
    }
};

#endif // OUTPUTCAPTURE_H