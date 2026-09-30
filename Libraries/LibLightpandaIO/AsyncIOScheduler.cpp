/*
 * Copyright (c) 2024, Ladybird Android Project
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "AsyncIOScheduler.h"
#include <AK/Debug.h>
#include <errno.h>
#include <unistd.h>

namespace Lightpanda {

NonnullOwnPtr<AsyncIOScheduler> AsyncIOScheduler::create()
{
    return adopt_own(*new AsyncIOScheduler());
}

AsyncIOScheduler::AsyncIOScheduler()
{
    m_epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (m_epoll_fd < 0) {
        dbgln("AsyncIOScheduler: Failed to create epoll fd: {}", strerror(errno));
    }
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
    dbgln_if(LIGHTPANDA_IO_DEBUG, "AsyncIOScheduler started with epoll fd {}", m_epoll_fd);
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

    struct epoll_event ev {};
    ev.events = static_cast<u32>(events);
    ev.data.fd = fd;

    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
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

    m_watchers.remove_all_matching([fd](auto& w) { return w.fd == fd; });
}

void AsyncIOScheduler::poll(int timeout_ms)
{
    if (!m_running || m_epoll_fd < 0)
        return;

    // Execute pending tasks first
    auto tasks = move(m_pending_tasks);
    m_pending_tasks.clear();
    for (auto& task : tasks) {
        if (task.callback)
            task.callback();
    }

    // Poll for IO events
    struct epoll_event events[MAX_EVENTS];
    int nfds = epoll_wait(m_epoll_fd, events, MAX_EVENTS, timeout_ms);

    if (nfds < 0) {
        if (errno != EINTR)
            dbgln("AsyncIOScheduler: epoll_wait failed: {}", strerror(errno));
        return;
    }

    for (int i = 0; i < nfds; ++i) {
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
