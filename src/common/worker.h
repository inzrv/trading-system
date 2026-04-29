#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>

class Worker
{
public:
    Worker() = default;
    virtual ~Worker() = default;

    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;
    Worker(Worker&&) = delete;
    Worker& operator=(Worker&&) = delete;

    void start()
    {
        std::lock_guard lock{m_mutex};
        if (m_running) {
            return;
        }

        m_running = true;
        m_thread = std::thread([this]() {
            run();
        });
    }

    void stop()
    {
        {
            std::lock_guard lock{m_mutex};
            if (!m_running) {
                return;
            }

            m_running = false;
        }

        m_cv.notify_all();
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

protected:
    virtual void run() = 0;

    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_running{false};

private:
    std::thread m_thread;
};
