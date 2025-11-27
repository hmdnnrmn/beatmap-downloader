#include "download_queue.h"
#include "download_manager.h"
#include "utils/logging.h"
#include "notification_manager.h"

DownloadQueue& DownloadQueue::Instance() {
    static DownloadQueue instance;
    return instance;
}

DownloadQueue::~DownloadQueue() {
    Stop();
}

void DownloadQueue::Start() {
    if (m_running) return;
    m_running = true;
    m_thread = std::thread(&DownloadQueue::WorkerThread, this);
    LogInfo("Download queue worker started");
}

void DownloadQueue::Stop() {
    if (!m_running) return;
    m_running = false;
    m_cv.notify_all();
    if (m_thread.joinable()) {
        m_thread.join();
    }
    LogInfo("Download queue worker stopped");
}



void DownloadQueue::Push(const std::wstring& id, bool isBeatmapId) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push({id, isBeatmapId});
    }
    m_cv.notify_one();
}

void DownloadQueue::WorkerThread() {
    while (m_running) {
        QueueItem item;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] { return !m_queue.empty() || !m_running; });

            if (!m_running && m_queue.empty()) {
                break;
            }

            item = m_queue.front();
            m_queue.pop();
        }

        DownloadBeatmap(item.id, item.isBeatmapId);
    }
}
