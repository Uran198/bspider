#ifndef _SPIDER_H_
#define _SPIDER_H_

#include <string>
#include <thread>
#include <queue>
#include <mutex>
#include <vector>
#include <unordered_set>

struct ScanURL {
    enum Status {
        SCAN_SUCCESS_FOUND,
        SCAN_SUCCESS_NOT_FOUND,
        SCAN_ERROR,
        SCANNING,
        PENDING
    };
    std::string url;
    Status status;
    std::string message;
    int id;
};

class Spider {
public:
    Spider();
    ~Spider();

    void set_thread_count(int value);
    void set_max_scan_urls(int value);
    void set_search_text(const std::string& text);
    void set_start_url(const std::string& text);

    ScanURL get_next_result();
    bool has_results();

    bool is_running() const;

    void reset();

    void start();
    void stop();

private:
    std::string m_search_text;
    ScanURL m_start_url;

    int m_thread_count;
    int m_max_scan_urls;
    std::vector<std::thread> m_threads;

    std::queue<ScanURL> m_results;
    std::mutex m_results_lock;

    int m_cnt_scanned_urls;
    int m_cnt_urls;
    std::queue<ScanURL> m_pending_urls;
    std::unordered_set<std::string> m_saw_url;
    std::mutex m_pending_urls_lock;

    bool m_stop_threads;

private:
    void search_urls();
};

#endif
