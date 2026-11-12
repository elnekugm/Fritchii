// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <exception>
#include <functional>
#include <iterator>
#include <mutex>
#include <stop_token>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "common/httplib.h"

#include <QObject>
#include <QString>
#include <fmt/format.h>

#include "common/fs/file.h"
#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/logging.h"
#include "common/thread.h"
#include "qt_common/config/uisettings.h"
#include "qt_common/util/boxart.h"

#ifdef YUZU_BUNDLED_OPENSSL
#include <openssl/cert.h>
#endif

namespace QtCommon::Boxart {

namespace {

// Eden's box art, which covers Switch 1 games.
constexpr char EDEN_BOXART_URL[] =
    "https://raw.githubusercontent.com/eden-emulator/boxart/refs/heads/master/img/";

constexpr std::size_t MaxQueued = 32;
constexpr std::size_t MaxCoverSize = 8 * 1024 * 1024;

// Older configurations stored URLs with the "//" of the scheme collapsed into "/" (the INI
// path normalization did that). Restore the missing slash so the URL can be parsed.
std::string FixUrl(std::string url) {
    const auto scheme = url.find(":/");
    if (scheme != std::string::npos && url.compare(scheme, 3, "://") != 0) {
        url.insert(scheme + 2, "/");
    }
    return url;
}

void ReplaceAll(std::string& text, std::string_view from, std::string_view to) {
    std::size_t position = 0;
    while ((position = text.find(from, position)) != std::string::npos) {
        text.replace(position, from.size(), to);
        position += to.size();
    }
}

// The repositories name box art after the title: "Hyrule Warriors: Age of Imprisonment" ->
// "hyrule-warriors-age-of-imprisonment". Accented letters lose the accent, other non-ASCII
// characters (trademark signs, curly quotes) are dropped.
std::string MakeCoverSlug(std::string_view title) {
    std::string slug{title};
    ReplaceAll(slug, "\xC3\xA9", "e"); // é
    ReplaceAll(slug, "\xC3\xA8", "e"); // è
    ReplaceAll(slug, "\xC3\xAA", "e"); // ê
    ReplaceAll(slug, "\xC3\xA1", "a"); // á
    ReplaceAll(slug, "\xC3\xA0", "a"); // à
    ReplaceAll(slug, "\xC3\xA2", "a"); // â
    ReplaceAll(slug, "\xC3\xAD", "i"); // í
    ReplaceAll(slug, "\xC3\xAC", "i"); // ì
    ReplaceAll(slug, "\xC3\xAE", "i"); // î
    ReplaceAll(slug, "\xC3\xB3", "o"); // ó
    ReplaceAll(slug, "\xC3\xB2", "o"); // ò
    ReplaceAll(slug, "\xC3\xB4", "o"); // ô
    ReplaceAll(slug, "\xC5\x8D", "o"); // ō
    ReplaceAll(slug, "\xC3\xBA", "u"); // ú
    ReplaceAll(slug, "\xC3\xB9", "u"); // ù
    ReplaceAll(slug, "\xC3\xBB", "u"); // û
    ReplaceAll(slug, "\xC3\xB1", "n"); // ñ
    ReplaceAll(slug, "\xC3\xA7", "c"); // ç

    std::string out;
    out.reserve(slug.size());
    for (char c : slug) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        } else if (c == ' ' || c == '_') {
            c = '-';
        }

        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            out += c;
        } else if (c == '-' && !out.empty() && out.back() != '-') {
            out += c;
        }
    }
    if (!out.empty() && out.back() == '-') {
        out.pop_back();
    }
    return out;
}

bool IsValidSlug(std::string_view slug) {
    // The repositories also hold the logo and the placeholder, which are not box art.
    if (slug.empty() || slug == "placeholder" || slug == "nemyu" || slug == "lichii" ||
        slug == "eden") {
        return false;
    }
    return std::all_of(slug.begin(), slug.end(), [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
               c == '-' || c == '_';
    });
}

bool IsImage(std::string_view data) {
    constexpr std::string_view Png{"\x89PNG\r\n\x1a\n", 8};
    constexpr std::string_view Jpeg{"\xFF\xD8\xFF", 3};
    return data.starts_with(Png) || data.starts_with(Jpeg);
}

// Fetches url into out. Returns the HTTP status, or 0 if the server could not be reached or did
// not send a PNG or JPEG image.
int Download(const std::string& url, std::string& out, std::stop_token stop_token) {
    const std::string target = FixUrl(url);
    const auto scheme_end = target.find("://");
    const auto path_start =
        scheme_end == std::string::npos ? scheme_end : target.find('/', scheme_end + 3);
    if (path_start == std::string::npos) {
        return 0;
    }

    // Try again when the server is temporarily unavailable or rate limits the request.
    constexpr int attempts = 3;
    for (int attempt = 0; attempt < attempts; ++attempt) {
        if (stop_token.stop_requested()) {
            return 0;
        }

        try {
            httplib::Client client(target.substr(0, path_start));
            client.set_connection_timeout(3);
            client.set_read_timeout(5);
            client.set_follow_location(true);

#ifdef YUZU_BUNDLED_OPENSSL
            client.load_ca_cert_store(kCert, sizeof(kCert));
#endif

            const std::stop_callback cancel{stop_token, [&client] { client.stop(); }};
            int status = 0;
            out.clear();
            const auto result = client.Get(
                target.substr(path_start),
                [&status](const httplib::Response& response) {
                    status = response.status;
                    if (status != 200) {
                        return true;
                    }
                    // Some hosts serve box art as application/octet-stream, so the payload is
                    // validated by its magic bytes instead of its Content-Type.
                    return response.get_header_value_u64("Content-Length") <= MaxCoverSize;
                },
                [&](const char* data, std::size_t length) {
                    if (status != 200) {
                        return true;
                    }
                    if (stop_token.stop_requested() || out.size() + length > MaxCoverSize) {
                        return false;
                    }
                    out.append(data, length);
                    return true;
                });
            if (!result) {
                return 0;
            }
            if (result->status == 200) {
                return IsImage(out) ? 200 : 0;
            }
            if ((result->status == 429 || result->status >= 500) && attempt + 1 < attempts) {
                std::this_thread::sleep_for(std::chrono::seconds(attempt + 1));
                continue;
            }
            return result->status;
        } catch (const std::exception& e) {
            if (attempt + 1 < attempts) {
                std::this_thread::sleep_for(std::chrono::seconds(attempt + 1));
                continue;
            }
            LOG_WARNING(Frontend, "Box art download from {} failed: {}", target, e.what());
            return 0;
        }
    }
    return 0;
}

bool Store(const std::filesystem::path& path, std::string_view data) {
    // Written next to its final name and renamed, so a partial file is never taken for box art.
    const auto temp = std::filesystem::path{path}.concat(".part");
    if (!Common::FS::CreateDirs(path.parent_path()) ||
        Common::FS::WriteStringToFile(temp, Common::FS::FileType::BinaryFile, data) !=
            data.size() ||
        !Common::FS::RenameFile(temp, path)) {
        Common::FS::RemoveFile(temp);
        return false;
    }
    return true;
}

class Downloader {
public:
    static Downloader& Instance() {
        static Downloader instance;
        return instance;
    }

    void Push(std::string slug, std::vector<std::string> urls, bool urgent) {
        if (!IsValidSlug(slug) || Common::FS::IsFile(GetCachePath(slug))) {
            return;
        }
        {
            std::scoped_lock lock{mutex};
            if (stopped) {
                return;
            }
            if (urgent) {
                const auto queued = std::find_if(queue.begin(), queue.end(),
                                                 [&](const Job& job) { return job.slug == slug; });
                if (queued != queue.end()) {
                    std::rotate(queue.begin(), queued, std::next(queued));
                    return;
                }
                if (!seen.insert(slug).second) {
                    return;
                }
                queue.push_front({std::move(slug), std::move(urls)});
            } else {
                // A full queue drops the request, the next game list refresh asks again.
                if (queue.size() >= MaxQueued || !seen.insert(slug).second) {
                    return;
                }
                queue.push_back({std::move(slug), std::move(urls)});
            }
            if (!worker.joinable()) {
                worker = std::jthread([this](std::stop_token stop_token) { Run(stop_token); });
            }
        }
        request_cv.notify_one();
    }

    void Stop() {
        std::jthread stopping;
        {
            std::scoped_lock lock{mutex};
            stopped = true;
            queue.clear();
            stopping = std::move(worker);
        }
        // Joined outside the lock, as the worker takes it to see the stop request.
    }

    void SetOnSaved(QObject* context, std::function<void(const std::string&)> callback) {
        std::scoped_lock lock{mutex};
        if (callback) {
            listeners.insert_or_assign(context, std::move(callback));
        } else {
            listeners.erase(context);
        }
    }

private:
    struct Job {
        std::string slug;
        std::vector<std::string> urls; ///< Tried in order
    };

    void Run(std::stop_token stop_token) {
        Common::SetCurrentThreadName("BoxartDownload");

        while (true) {
            Job job;
            {
                std::unique_lock lock{mutex};
                request_cv.wait(lock, stop_token, [this] { return !queue.empty(); });
                if (stop_token.stop_requested()) {
                    return;
                }
                job = std::move(queue.front());
                queue.pop_front();
            }

            if (!Fetch(job, stop_token)) {
                std::scoped_lock lock{mutex};
                seen.erase(job.slug);
            }
        }
    }

    // False if the box art may still exist, so a later request should try again.
    bool Fetch(const Job& job, std::stop_token stop_token) {
        const auto path = GetCachePath(job.slug);
        if (Common::FS::IsFile(path)) {
            return true;
        }

        bool all_missing = true;
        int last_status = 0;
        std::string last_url;
        for (const auto& url : job.urls) {
            if (stop_token.stop_requested()) {
                return false;
            }
            std::string data;
            const int status = Download(url, data, stop_token);
            if (status == 200) {
                if (!Store(path, data)) {
                    LOG_WARNING(Frontend, "Could not save box art to {}",
                                Common::FS::PathToUTF8String(path));
                    return false;
                }
                LOG_INFO(Frontend, "Saved box art from {}", url);
                Notify(job.slug);
                return true;
            }
            last_status = status;
            last_url = url;
            all_missing = all_missing && status == 404;
        }
        if (!all_missing) {
            LOG_WARNING(Frontend, "Box art for '{}' not downloaded ({} returned status {})",
                        job.slug, last_url, last_status);
        }
        return all_missing;
    }

    // Posted under the lock, so a listener that was just removed is never called.
    void Notify(const std::string& slug) {
        std::scoped_lock lock{mutex};
        for (const auto& [context, callback] : listeners) {
            QMetaObject::invokeMethod(
                context, [callback, slug] { callback(slug); }, Qt::QueuedConnection);
        }
    }

    std::mutex mutex;
    std::condition_variable_any request_cv;
    std::deque<Job> queue;
    std::unordered_set<std::string> seen; ///< Queued, saved or missing everywhere this session
    bool stopped = false;
    std::unordered_map<QObject*, std::function<void(const std::string&)>> listeners;
    std::jthread worker;
};

} // namespace

std::filesystem::path GetCachePath(std::string_view slug) {
    if (!IsValidSlug(slug)) {
        return {};
    }
    return Common::FS::GetEdenPath(Common::FS::EdenPath::EdenDir) / "boxart" /
           fmt::format("{}.png", slug);
}

std::vector<std::string> GetCoverUrls(std::string_view slug) {
    std::vector<std::string> urls;
    std::string base_url = FixUrl(UISettings::values.boxart_url.GetValue());
    if (!base_url.empty()) {
        if (!base_url.ends_with('/')) {
            base_url += '/';
        }
        urls.push_back(fmt::format("{}{}.png", base_url, slug));
    }
    urls.push_back(fmt::format("{}{}.png", EDEN_BOXART_URL, slug));
    return urls;
}

QImage LoadCachedCover([[maybe_unused]] u64 program_id, const std::string& title) {
    const auto path = GetCachePath(GetSlug(program_id, title));
    if (path.empty() || !Common::FS::IsFile(path)) {
        return {};
    }
    return QImage(QString::fromStdString(Common::FS::PathToUTF8String(path)));
}

std::string GetSlug([[maybe_unused]] u64 program_id, const std::string& title) {
    std::string slug = MakeCoverSlug(title);
    return IsValidSlug(slug) ? slug : std::string{};
}

namespace {

// Titles of Switch 2 re-releases often end with an edition tag that the box art repositories do
// not use, so the same cover is also looked up under the plain title.
std::vector<std::string> GetSlugVariants(const std::string& title) {
    std::vector<std::string> variants;
    std::string slug = MakeCoverSlug(title);
    if (!IsValidSlug(slug)) {
        return variants;
    }
    variants.push_back(slug);

    constexpr std::string_view suffixes[] = {
        "-nintendo-switch-2-edition", "-switch-2-edition", "-nintendo-switch-2"};
    for (const auto suffix : suffixes) {
        if (slug.ends_with(suffix)) {
            const std::string base = slug.substr(0, slug.size() - suffix.size());
            if (IsValidSlug(base) &&
                std::find(variants.begin(), variants.end(), base) == variants.end()) {
                variants.push_back(base);
            }
        }
    }
    return variants;
}

} // namespace

void Prefetch(u64 program_id, const std::string& title, bool urgent) {
    // Only games the repositories may have: Switch 1 and Switch 2 titles.
    const u64 platform = program_id >> 56;
    if (platform != 0x01 && platform != 0x04) {
        return;
    }
    std::vector<std::string> slugs = GetSlugVariants(title);
    if (slugs.empty()) {
        return;
    }
    const std::string slug = slugs.front();
    LOG_INFO(Frontend, "Box art: looking up '{}' (program_id={:016X})", slug, program_id);

    std::vector<std::string> urls;
    for (const auto& variant : slugs) {
        for (auto& url : GetCoverUrls(variant)) {
            if (std::find(urls.begin(), urls.end(), url) == urls.end()) {
                urls.push_back(std::move(url));
            }
        }
    }
    Downloader::Instance().Push(slug, std::move(urls), urgent);
}

void Save(std::string slug, std::string url) {
    Downloader::Instance().Push(std::move(slug), {std::move(url)}, false);
}

void OnSaved(QObject* context, std::function<void(const std::string& slug)> on_saved) {
    Downloader::Instance().SetOnSaved(context, std::move(on_saved));
}

void Shutdown() {
    Downloader::Instance().Stop();
}

} // namespace QtCommon::Boxart
