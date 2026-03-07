

#include <string>
#include <vector>

class FileBuffer
{
public:
    bool load(const char *path);
   

    const char *c_str() const
    {
        if (data.empty())
            return nullptr;
        return reinterpret_cast<const char *>(data.data());
    }

    size_t size() const
    {
        if (data.empty())
            return 0;
        return data.size() - 1;
    }

    std::string toString() const
    {
        if (data.empty())
            return "";
        return std::string(reinterpret_cast<const char *>(data.data()), size());
    }

private:
    std::vector<unsigned char> data;
};