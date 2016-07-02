#include "Spider.h"

#include <cpr/cpr.h>

#include <chrono>
#include <sstream>

const std::string url_prefix = "http://";

Spider::Spider()
    : m_thread_count(0)
    , m_max_scan_urls(0)
    , m_cnt_scanned_urls(0)
    , m_cnt_urls(0)
    , m_stop_threads(true)
{
}

Spider::~Spider()
{
    stop();
}

void Spider::set_thread_count(int value)
{
    m_thread_count = value;
}

void Spider::set_search_text(const std::string& text)
{
    m_search_text = text;
}

void Spider::set_start_url(const std::string& text)
{
    m_start_url.id = m_cnt_urls++;
    m_start_url.status = ScanURL::PENDING;
    m_start_url.url = text;
    m_start_url.message = "Pending";
}

void Spider::set_max_scan_urls(int value)
{
    m_max_scan_urls = value;
}

void Spider::reset()
{
    m_cnt_scanned_urls = 0;
    m_cnt_urls = 0;
    {
        std::lock_guard<std::mutex> lock_(m_pending_urls_lock);
        while (!m_pending_urls.empty())
            m_pending_urls.pop();
        m_saw_url.clear();
    }
    {
        std::lock_guard<std::mutex> lock(m_results_lock);
        while (!m_results.empty())
            m_results.pop();
    }
}

bool Spider::has_results()
{
    std::lock_guard<std::mutex> lock(m_results_lock);
    return m_results.size() > 0;
}

ScanURL Spider::get_next_result()
{
    const std::chrono::duration<double, std::milli> sleep_time(50);
    while (!has_results())
        std::this_thread::sleep_for(sleep_time);
    std::lock_guard<std::mutex> lock(m_results_lock);
    ScanURL ret = m_results.front();
    m_results.pop();
    return ret;
}

bool Spider::is_running() const
{
    return !m_stop_threads;
}

void Spider::start()
{
    if (!m_stop_threads)
        return;
    stop();
    m_stop_threads = false;
    m_cnt_urls++; // one for start

    m_pending_urls.push(m_start_url);
    m_results.push(m_start_url);

    m_threads.resize(m_thread_count);
    for (int i = 0; i < m_thread_count; ++i) {
        m_threads[i] = std::thread(&Spider::search_urls, this);
    }
}

void Spider::stop()
{
    m_stop_threads = true;
    for (int i = 0; i < (int)m_threads.size(); ++i) {
        m_threads[i].join();
    }
    m_threads.clear();
    reset();
}

std::vector<std::string> extract_urls(const std::string& text)
{
    const std::string valid_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~:/?#[]@!$&'()*+,;=%";
    bool valid_char[256];

    for (int i = 0; i < 256; ++i) {
        valid_char[i] = false;
    }
    for (char c : valid_chars) {
        valid_char[c] = true;
    }
    std::vector<size_t> positions;

    size_t pos = text.find(url_prefix, 0);
    while (pos != std::string::npos) {
        positions.push_back(pos);
        pos = text.find(url_prefix, pos + 1);
    }

    std::vector<std::string> res;
    for (auto pos : positions) {
        std::string cur;
        while (pos < text.size() && valid_char[text[pos]]) {
            cur += text[pos];
            pos++;
        }
        res.push_back(cur);
    }
    return res;
}

std::vector<std::string> scan_url(ScanURL& url, const std::string& search_text)
{
    auto response = cpr::Get(cpr::Url{ url.url }, cpr::Timeout{1000});
    std::string text = response.text;

    if (response.status_code >= 400) {
        url.status = ScanURL::SCAN_ERROR;
        std::stringstream ss;
        ss << "Error: " << response.status_code;
        url.message = ss.str();
    } else if (text.find(search_text) != std::string::npos) {
        url.status = ScanURL::SCAN_SUCCESS_FOUND;
        url.message = "Found";
    } else {
        url.status = ScanURL::SCAN_SUCCESS_NOT_FOUND;
        url.message = "Not found";
    }

    std::vector<std::string> ret = extract_urls(text);
    return ret;
}

void Spider::search_urls()
{
    while (true) {
        if (m_stop_threads)
            break;

        ScanURL ret;
        {
            std::lock_guard<std::mutex> lock(m_pending_urls_lock);
            if (m_pending_urls.empty()) {
                continue;
            }
            ret = m_pending_urls.front();
            ret.status = ScanURL::SCANNING;
            ret.message = "Scanning";
            m_pending_urls.pop();
        }
        {
            std::lock_guard<std::mutex> lock(m_results_lock);
            m_results.push(ret);
        }

        if (m_stop_threads)
            break;
        std::vector<std::string> nxt_urls = scan_url(ret, m_search_text);
        {
            std::lock_guard<std::mutex> lock(m_results_lock);
            m_results.push(ret);
            m_cnt_scanned_urls++;
            if (m_cnt_scanned_urls >= m_max_scan_urls) {
                m_stop_threads = true;
                break;
            }
        }

        for (int i = 0; i < (int)nxt_urls.size(); ++i) {
            if (m_stop_threads)
                break;
            {
                std::lock_guard<std::mutex> lock_(m_pending_urls_lock);
                if (m_saw_url.count(nxt_urls[i]) > 0)
                    continue;
                m_saw_url.insert(nxt_urls[i]);
            }
            ScanURL nxt;
            nxt.url = nxt_urls[i];
            nxt.status = ScanURL::PENDING;
            nxt.message = "Pending";

            nxt.id = m_cnt_urls++;
            if (m_cnt_urls <= m_max_scan_urls) {
                {
                    std::lock_guard<std::mutex> lock_(m_pending_urls_lock);
                    m_pending_urls.push(nxt);
                }
                std::lock_guard<std::mutex> lock(m_results_lock);
                m_results.push(nxt);
            }
        }
    }
}
