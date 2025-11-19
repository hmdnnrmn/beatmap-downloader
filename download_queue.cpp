#include "download_queue.h"
#include "download_manager.h"
#include "logging.h"
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

void DownloadQueue::Push(const std::wstring& beatmapId) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(beatmapId);
    }
    LogInfo("Added beatmap to download queue: " + std::string(beatmapId.begin(), beatmapId.end()));
    NotificationManager::Instance().ShowNotification(L"Download Queued", L"Beatmap added to queue");
    m_cv.notify_one();
}

void DownloadQueue::WorkerThread() {
    while (m_running) {
        std::wstring beatmapId;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] { return !m_queue.empty() || !m_running; });

            if (!m_running && m_queue.empty()) break;

            beatmapId = m_queue.front();
            m_queue.pop();
        }

        if (!beatmapId.empty()) {
            DownloadBeatmap(beatmapId);
        }
    }
}
