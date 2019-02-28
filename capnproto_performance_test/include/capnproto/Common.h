#pragma once

#include <zmq.h>

#include <iostream>
#include <string>

namespace capnproto
{
    enum CommType {
        UDP,
        TCP,
        IPC
    };

/**
 * Checks the return code and reports an error if present.
 * If abortIfError is set to true, it also aborts the process.
 */
inline void check(int returnCode, std::string methodName)
{
    if (returnCode != 0) {
        std::cerr << methodName << " returned: " << errno << " - " << zmq_strerror(errno) << std::endl;
    }
}
} // namespace capnproto