#include "filebuffer.hpp"
#include "utils.hpp"
#include "interpreter.hpp"
#include <SDL2/SDL.h> 
 
 

bool FileBuffer::load(const char *path)

{
    data.clear();

   unsigned int fileSize = 0;
    unsigned char *fileData = Utils::LoadFileData(path, &fileSize);
    if (!fileData || fileSize <= 0)
    {
        if (fileData)
           SDL_free(fileData);
        return false;
    }

    data.assign(fileData, fileData + fileSize);
    data.push_back(0); // Keep NUL terminator for C-style loaders.
    SDL_free(fileData);
    return true;
}