#include "pch.h"

#include "core/log.h"
#include "core/sync.h"

namespace core {

    // if the wait sync functions have been blocking for more than this amount of time
    // a warning message will be logged to the console to notify the user of this wait.
    constexpr GLuint64 warn_threshold = static_cast<GLuint64>(1e10);  // 10 seconds

    Sync::Sync(GLuint id) : id(id) {
        sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        CORE_ASERT(sync != nullptr, "Unable to create a fence sync object");
    }

    Sync::~Sync() {
        glDeleteSync(sync);
        sync = nullptr;
    }

    bool Sync::Signaled() {
        /* this function queries the signal state of the sync object without blocking
           this allows us to work on sth else before the sync object is signaled, e.g.

           > auto fence = Sync();
           > while (!fence.Signaled()) {
           >     DoWork();
           > }
        */

        GLint status = GL_UNSIGNALED;
        glGetSynciv(sync, GL_SYNC_STATUS, sizeof(GLint), NULL, &status);
        return (status == GL_SIGNALED) ? true : false;
    }

    void Sync::ClientWaitSync(GLuint64 timeout) {
        // this function will block the CPU until the sync object is signaled
        // automatic flush on the first wait
        GLenum status = glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, timeout);

        bool warned = false;
        GLuint64 wait_time = timeout;

        // keep trying until either the sync object is signaled or the wait errored out
        while (status == GL_TIMEOUT_EXPIRED) {
            status = glClientWaitSync(sync, 0, timeout);  // subsequent calls don't need the flush bit
            wait_time += timeout;  // in nanoseconds

            if (!warned && wait_time > warn_threshold) {
                CORE_WARN("Sync object {0} has been hanging for over 10 secs on the client!", id);
                warned = true;
            }
        }

        // now that we are here, the sync object is either signaled or wait failed
        if (status == GL_WAIT_FAILED) {
            CORE_ERROR("An error occurred while waiting on sync object {0}", id);
        }
    }

    void Sync::ServerWaitSync() {
        // this function is only useful when you have multiple threads with multiple contexts.
        // it won't halt the CPU but will block the driver from sending commands to the GPU's
        // command queue until the sync object is signaled

        glFlush();  // make sure the sync object is flushed to the GPU to avoid an infinite loop

        bool warned = false;
        GLuint64 wait_time = 0;

        while (!Signaled()) {
            glWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
            wait_time += GetServerTimeout();  // in nanoseconds

            if (!warned && wait_time > warn_threshold) {
                CORE_WARN("Sync object {0} has been hanging for over 10 secs on the server!", id);
                warned = true;
            }
        }
    }

    GLint64 Sync::GetServerTimeout() {
        // the implementation-dependent server wait timeout
        static GLint64 max_server_timeout = -1;
        if (max_server_timeout < 0) {
            glGetInteger64v(GL_MAX_SERVER_WAIT_TIMEOUT, &max_server_timeout);
        }
        return max_server_timeout;
    }

    void Sync::WaitFlush() {
        glFlush();  // wait until all commands issued so far are flushed to the GPU
    }

    void Sync::WaitFinish() {
        // calling this function every frame can drastically reduce performance
        // very much like how excessive idling can damage your vehicle's engine
        glFinish();  // wait until all commands issued so far are fully executed by the GPU
    }

}
