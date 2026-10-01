/*
 * Copyright (c) 2024, Ladybird Android Project
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Inspired by lightpanda-io/browser async IO architecture.
 * Implements epoll-based non-blocking IO scheduling for Android.
 */

#pragma once

#include <AK/Function.h>
#include <AK/NonnullOwnPtr.h>
#include <AK/Vector.h>
#include <AK/kmalloc.h>
#include <sys/epoll.h>

namespace Lightpanda {

class AsyncIOScheduler {
public:
    AK_ALLOC_WITH_KMALLOC;

    static NonnullOwnPtr<AsyncIOScheduler> create();
    ~AsyncIOScheduler();

    using TaskCallback = Function<void()>;

    // Schedule a task to run on the next IO cycle
    void schedule(TaskCallback callback);

    // Run one iteration of the event loop (non-blocking)
    void poll(int timeout_ms = 0);

    // Start/stop the scheduler
    void start();
    void stop();
    bool is_running() const { return m_running; }

    // Register an fd for monitoring
    enum class EventType : u32 {
        Read = EPOLLIN,
        Write = EPOLLOUT,
        Error = EPOLLERR | EPOLLHUP
    };

    using FDCallback = Function<void(EventType)>;
    bool register_fd(int fd, EventType events, FDCallback callback);
    void unregister_fd(int fd);

private:
    AsyncIOScheduler();

    struct PendingTask {
        TaskCallback callback;
    };

    struct FDWatcher {
        int fd;
        FDCallback callback;
        EventType events;
    };

    int m_epoll_fd { -1 };
    bool m_running { false };
    Vector<PendingTask> m_pending_tasks;
    Vector<FDWatcher> m_watchers;
    static constexpr int MAX_EVENTS = 64;
};

} // namespace Lightpanda
