#pragma once

#ifndef BUGL_VERSION_STRING
#define BUGL_VERSION_STRING "2.0.0"
#endif

#ifndef BUGL_VERSION_GIT
#define BUGL_VERSION_GIT "unknown"
#endif

namespace bugl
{
namespace version
{
inline const char *string()
{
    return BUGL_VERSION_STRING;
}

inline const char *git()
{
    return BUGL_VERSION_GIT;
}
} // namespace version
} // namespace bugl
