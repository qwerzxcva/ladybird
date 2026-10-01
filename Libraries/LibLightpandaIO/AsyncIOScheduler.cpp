/*
 * Copyright (c) 2024, Ladybird Android Project
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "AsyncIOScheduler.h"
#include <AK/Debug.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

namespace Lightpanda {

NonnullOwnPtr<AsyncIOScheduler> AsyncIOScheduler::create()
{
    // make<T>() cannot be used here: it is a free function and the constructor is private.
    return adopt_own(*new AsyncIOScheduler());
}

AsyncIOScheduler::AsyncIOScheduler()
{
    m_epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (m_epoll_fd < 0)
        dbgln("AsyncIOScheduler: Failed to create epoll fd: {}", strerror(errno));
}

AsyncIOScheduler::~AsyncIOScheduler()
{
    stop();
    if (m_epoll_fd >= 0)
        close(m_epoll_fd);
}

void AsyncIOScheduler::start()
{
    m_running = true;
}

void AsyncIOScheduler::stop()
{
    m_running = false;
    m_pending_tasks.clear();
    m_watchers.clear();
}

void AsyncIOScheduler::schedule(TaskCallback callback)
{
    m_pending_tasks.append({ move(callback) });
}

bool AsyncIOScheduler::register_fd(int fd, EventType events, FDCallback callback)
{
    if (m_epoll_fd < 0)
        return false;

    struct epoll_event event {};
    event.events = static_cast<u32>(events);
    event.data.fd = fd;

    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0) {
        dbgln("AsyncIOScheduler: Failed to add fd {} to epoll: {}", fd, strerror(errno));
        return false;
    }

    m_watchers.append({ fd, move(callback), events });
    return true;
}

void AsyncIOScheduler::unregister_fd(int fd)
{
    if (m_epoll_fd >= 0)
        epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);

    m_watchers.remove_all_matching([fd](auto& watcher) { return watcher.fd == fd; });
}

void AsyncIOScheduler::poll(int timeout_ms)
{
    if (!m_running || m_epoll_fd < 0)
        return;

    auto pending_tasks = move(m_pending_tasks);
    m_pending_tasks.clear();
    for (auto& task : pending_tasks) {
        if (task.callback)
            task.callback();
    }

    struct epoll_event events[MAX_EVENTS];
    int ready_count = epoll_wait(m_epoll_fd, events, MAX_EVENTS, timeout_ms);

    if (ready_count < 0) {
        if (errno != EINTR)
            dbgln("AsyncIOScheduler: epoll_wait failed: {}", strerror(errno));
        return;
    }

    for (int i = 0; i < ready_count; ++i) {
        int fd = events[i].data.fd;
        auto event_type = static_cast<EventType>(events[i].events);

        for (auto& watcher : m_watchers) {
            if (watcher.fd == fd && watcher.callback) {
                watcher.callback(event_type);
                break;
            }
        }
    }
}

} // namespace Lightpanda
