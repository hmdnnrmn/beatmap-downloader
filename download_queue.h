#pragma once
#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

class DownloadQueue {
public:
    static DownloadQueue& Instance();

    void Start();
    void Stop();
    void Push(const std::wstring& beatmapId);

private:
    DownloadQueue() = default;
    ~DownloadQueue();
    DownloadQueue(const DownloadQueue&) = delete;
    DownloadQueue& operator=(const DownloadQueue&) = delete;

    void WorkerThread();

    std::queue<std::wstring> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
};
