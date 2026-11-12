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

// The LaunchBox Games Database holds art for Switch 2 games as well. Its search and detail
// pages are plain HTML, so the front cover is picked out of the page by its alt text.
constexpr char LAUNCHBOX_HOST[] = "https://gamesdb.launchbox-app.com";
constexpr char LAUNCHBOX_IMAGE_HOST[] = "https://gamesdb-images.launchbox.gg";
constexpr char LAUNCHBOX_IMAGE_HOST_ALT[] = "https://images.launchbox-app.com";

constexpr std::size_t MaxQueued = 32;
constexpr std::size_t MaxCoverSize = 8 * 1024 * 1024;
constexpr std::size_t MaxPageSize = 4 * 1024 * 1024;

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

namespace LaunchBox {

// The search page takes the words of the title, so the name of a dumped update ("starfox-update")
// is cleaned up before it is searched for.
void CleanTitle(std::string& title) {
    // Words that name the file rather than the game.
    constexpr std::string_view Noise[] = {
        "update",  "updates", "base",   "demo",  "version", "full", "edition",
        "jp",     "usa",     "eur",    "pal",   "ntsc",   "nsonly", "rev",
        "nintendo", "switch", "switch2", "ns2",  "ns2e",  "nsw",   "sw2",
        "of",     "the",     "not",
    };
    // Words that are written together in the name of the file.
    constexpr std::pair<std::string_view, std::string_view> Expanded[] = {
        {"starfox", "star fox"},
        {"fastfusion", "fast fusion"},
        {"mariotennisfever", "mario tennis fever"},
        {"supermariobroswonder", "super mario bros wonder"},
        {"supermariobros", "super mario bros"},
        {"mariokartworld", "mario kart world"},
        {"mkw", "mario kart world"},
        {"yoshiandthemysteriousbook", "yoshi and the mysterious book"},
        {"pokemonlegendsz-a", "pokemon legends z-a"},
        {"thelegendofzelda", "the legend of zelda"},
        {"dragxdrive", "drag x drive"},
        {"globaljam", "global jam racing"},
        {"dkb", "donkey kong bananza"},
        {"splatoonraiders", "splatoon raiders"},
    };

    std::string out;
    for (std::size_t i = 0; i < title.size();) {
        const auto space = title.find_first_of(" -_+", i);
        const auto word = title.substr(i, space == std::string::npos ? std::string::npos : space - i);
        i = space == std::string::npos ? title.size() : space + 1;
        if (word.empty()) {
            continue;
        }

        std::string lower;
        lower.reserve(word.size());
        for (char c : word) {
            lower += (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
        }

        // Version numbers ("v133") and program IDs ("0400a3a01ee4c000") name the file, not the game.
        bool noise = false;
        if (lower.size() > 1 && lower[0] == 'v') {
            noise = true;
            for (std::size_t c = 1; c < lower.size(); ++c) {
                if (lower[c] < '0' || lower[c] > '9') {
                    noise = false;
                    break;
                }
            }
        }
        if (lower.size() >= 12) {
            bool id = true;
            for (char c : lower) {
                if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
                    id = false;
                    break;
                }
            }
            noise = noise || id;
        }
        for (const auto& word_noise : Noise) {
            if (lower == word_noise) {
                noise = true;
            }
        }
        // A bare number behind a platform word is its generation ("switch 2"), not part of the name.
        if (lower == "2" || lower == "1") {
            noise = true;
        }
        if (noise) {
            continue;
        }

        for (const auto& [from, to] : Expanded) {
            if (lower == from) {
                lower = std::string{to};
            }
        }
        if (!out.empty()) {
            out += ' ';
        }
        out += lower;
    }
    title = std::move(out);
}

// Percent-encodes the query for the search URL.
std::string UrlEncode(std::string_view text) {
    static constexpr char Hex[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~') {
            out += c;
        } else if (c == ' ') {
            out += '+';
        } else {
            out += '%';
            out += Hex[(static_cast<unsigned char>(c) >> 4) & 0xF];
            out += Hex[static_cast<unsigned char>(c) & 0xF];
        }
    }
    return out;
}

// The words of a name, without punctuation and with the HTML entities of the search page decoded.
std::vector<std::string> Words(std::string_view name) {
    std::vector<std::string> words;
    std::string current;
    for (std::size_t i = 0; i < name.size();) {
        const char c = name[i];
        if (c == '&') {
            // "&#xE9;" and the named entities of the titles.
            const auto end = name.find(';', i);
            if (end != std::string_view::npos && end - i <= 10) {
                const auto entity = name.substr(i + 1, end - i - 1);
                std::string decoded;
                if (entity.starts_with("#x") || entity.starts_with("#X")) {
                    const auto value = std::strtol(entity.c_str() + 2, nullptr, 16);
                    if (value > 0 && value < 128) {
                        decoded += static_cast<char>(value);
                    }
                } else if (entity == "amp") {
                    decoded = "&";
                } else if (entity == "quot") {
                    decoded = "\"";
                } else if (entity == "apos" || entity == "#39") {
                    decoded = "'";
                }
                if (!decoded.empty()) {
                    if (decoded != ' ') {
                        current += decoded;
                    }
                    i = end + 1;
                    continue;
                }
            }
        }
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
            current += (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
        } else if (!current.empty()) {
            words.push_back(std::move(current));
            current.clear();
        }
        ++i;
    }
    if (!current.empty()) {
        words.push_back(std::move(current));
    }

    // The accented letters lose their accent, so that they match the plain spelling.
    for (auto& word : words) {
        std::size_t out = 0;
        for (std::size_t i = 0; i < word.size();) {
            const unsigned char c = static_cast<unsigned char>(word[i]);
            if (c < 0x80) {
                word[out++] = word[i];
                ++i;
                continue;
            }
            // The two and three byte UTF-8 letters of the Latin range.
            std::size_t length = (c & 0xE0) == 0xC0 ? 2 : 3;
            if (i + length > word.size()) {
                break;
            }
            char plain = 0;
            switch (word[i + 1]) {
            case 0x89: plain = 'e'; break; // e with an acute accent
            case 0x92: plain = 'e'; break; // e with a grave accent
            case 0x9A: plain = 'e'; break; // e with a circumflex
            case 0xA1: plain = 'a'; break; // a with an acute accent
            case 0xA0: plain = 'a'; break; // a with a grave accent
            case 0xA2: plain = 'a'; break; // a with a circumflex
            case 0xAD: plain = 'i'; break; // i with an acute accent
            case 0xAC: plain = 'i'; break; // i with a grave accent
            case 0xAE: plain = 'i'; break; // i with a circumflex
            case 0xB3: plain = 'o'; break; // o with an acute accent
            case 0xB2: plain = 'o'; break; // o with a grave accent
            case 0xB4: plain = 'o'; break; // o with a circumflex
            case 0xBA: plain = 'u'; break; // u with an acute accent
            case 0xB9: plain = 'u'; break; // u with a grave accent
            case 0xBB: plain = 'u'; break; // u with a circumflex
            case 0xB1: plain = 'n'; break; // n with a tilde
            case 0xA7: plain = 'c'; break; // c with a cedilla
            default: break;
            }
            if (plain != 0) {
                word[out++] = plain;
            }
            i += length;
        }
        word.resize(out);
    }
    return words;
}
// game that the candidate names, with the words of the candidate that the game does not name
// counted against it. A candidate that names the game exactly scores 1.
float Score(const std::string& candidate, const std::vector<std::string>& wanted) {
    const auto words = Words(candidate);
    if (words.empty() || wanted.empty()) {
        return 0.0f;
    }
    std::size_t hits = 0;
    for (const auto& word : wanted) {
        for (const auto& other : words) {
            if (word == other) {
                ++hits;
                break;
            }
        }
    }
    const float matched = static_cast<float>(hits) / static_cast<float>(wanted.size());
    const float extra = static_cast<float>(words.size() - hits) / static_cast<float>(words.size());
    return matched - extra * 0.5f;
}

// Fetches a page of the LaunchBox site. Returns the HTTP status.
int FetchPage(const std::string& url, std::string& out, std::stop_token stop_token) {
    const auto scheme_end = url.find("://");
    if (scheme_end == std::string::npos) {
        return 0;
    }
    const auto path_start = url.find('/', scheme_end + 3);
    if (path_start == std::string::npos) {
        return 0;
    }

    try {
        httplib::Client client(url.substr(0, path_start));
        client.set_connection_timeout(5);
        client.set_read_timeout(10);
        client.set_follow_location(true);

#ifdef YUZU_BUNDLED_OPENSSL
        client.load_ca_cert_store(kCert, sizeof(kCert));
#endif

        const std::stop_callback cancel{stop_token, [&client] { client.stop(); }};
        int status = 0;
        out.clear();
        const auto result = client.Get(
            url.substr(path_start),
            [&status](const httplib::Response& response) {
                status = response.status;
                if (status != 200) {
                    return true;
                }
                return response.get_header_value_u64("Content-Length") <= MaxPageSize;
            },
            [&](const char* data, std::size_t length) {
                if (status != 200) {
                    return true;
                }
                if (stop_token.stop_requested() || out.size() + length > MaxPageSize) {
                    return false;
                }
                out.append(data, length);
                return true;
            });
        if (!result) {
            return 0;
        }
        return result->status;
    } catch (const std::exception& e) {
        LOG_DEBUG(Frontend, "LaunchBox request to {} failed: {}", url, e.what());
        return 0;
    }
}

// The URL of the game's page in the LaunchBox Games Database, or an empty string when the game
// is not in the database.
std::string FindGamePage(const std::string& title, std::stop_token stop_token) {
    std::string query = title;
    CleanTitle(query);
    if (query.empty()) {
        return {};
    }

    std::string page;
    const std::string url =
        fmt::format("{}/games/results/{}", LAUNCHBOX_HOST, UrlEncode(query));
    if (FetchPage(url, page, stop_token) != 200) {
        return {};
    }

    // Every result links to its page and names the game in the alt text of its picture.
    const auto wanted = Words(query);
    std::string best_page;
    float best_score = 0.0f;
    std::size_t pos = 0;
    while ((pos = page.find("/games/details/", pos)) != std::string::npos) {
        const auto slug_start = pos + std::string_view{"/games/details/"}.size();
        const auto slug_end = page.find('"', slug_start);
        if (slug_end == std::string::npos) {
            break;
        }
        const std::string slug = page.substr(slug_start, slug_end - slug_start);

        // The name of the game is in the alt text of the picture of the card, right after the
        // link to the page of the game.
        std::string name;
        const auto alt = page.find("alt=\"", slug_end);
        if (alt != std::string::npos && alt < slug_end + 400) {
            const auto alt_end = page.find('"', alt + 5);
            if (alt_end != std::string::npos) {
                name = page.substr(alt + 5, alt_end - alt - 5);
            }
        }

        const float score = Score(name, wanted);
        if (score > best_score) {
            best_score = score;
            best_page = fmt::format("{}/games/details/{}", LAUNCHBOX_HOST, slug);
        }
        pos = slug_end;
    }

    if (best_score < 0.34f) {
        return {};
    }
    return best_page;
}

// The URL of the front cover of the game on its LaunchBox page.
std::string FindBoxUrl(const std::string& game_page, std::stop_token stop_token) {
    std::string page;
    if (FetchPage(game_page, page, stop_token) != 200) {
        return {};
    }

    // The covers are the pictures whose alt text names the type of the picture and its region.
    // The North American cover is preferred, then the world and the European ones.
    const auto rank = [](std::string_view region) {
        if (region == "North America" || region == "United States" || region == "USA") {
            return 3;
        }
        if (region == "World") {
            return 2;
        }
        if (region == "Europe" || region == "United Kingdom" || region == "Germany") {
            return 1;
        }
        return 0;
    };

    std::string best_url;
    int best_rank = -1;
    std::size_t pos = 0;
    while ((pos = page.find("<img", pos)) != std::string::npos) {
        const auto tag_end = page.find('>', pos);
        if (tag_end == std::string::npos) {
            break;
        }
        const std::string tag = page.substr(pos, tag_end - pos);
        pos = tag_end + 1;

        const auto src = tag.find("src=\"");
        if (src == std::string::npos) {
            continue;
        }
        const auto src_start = src + 5;
        const auto src_end = tag.find('"', src_start);
        if (src_end == std::string::npos) {
            continue;
        }
        const std::string image = tag.substr(src_start, src_end - src_start);
        if (!image.starts_with("http")) {
            continue;
        }

        const auto alt = tag.find("alt=\"");
        if (alt == std::string::npos) {
            continue;
        }
        const auto alt_start = alt + 5;
        const auto alt_end = tag.find('"', alt_start);
        if (alt_end == std::string::npos) {
            continue;
        }
        const std::string text = page.substr(alt_start, alt_end - alt_start);

        // The alt text reads "<game> - <type> (<region>) - <width>x<height>".
        const auto type_pos = text.find(" - ");
        if (type_pos == std::string::npos) {
            continue;
        }
        const auto rest = text.substr(type_pos + 3);
        std::string type = rest.substr(0, rest.find(" ("));
        const auto region_pos = rest.find(" (");
        std::string region;
        if (region_pos != std::string::npos) {
            const auto region_end = rest.find(')', region_pos);
            if (region_end != std::string::npos) {
                region = rest.substr(region_pos + 2, region_end - region_pos - 2);
            }
        }
        if (type != "Box - Front" && type != "Box - Front - Reconstructed" && type != "Box - 3D") {
            continue;
        }

        const int current = rank(region);
        if (current > best_rank) {
            best_rank = current;
            best_url = image;
        }
    }

    if (best_url.empty()) {
        return {};
    }
    // The pictures are served by two hosts, the second one holds the older files.
    if (best_url.starts_with("https://images.launchbox-app.com/")) {
        std::string file = best_url.substr(std::string_view{"https://images.launchbox-app.com/"}.size());
        if (file.starts_with('/')) {
            file.erase(0, 1);
        }
        best_url = fmt::format("{}/{}", LAUNCHBOX_IMAGE_HOST, file);
    }
    return best_url;
}

} // namespace LaunchBox

class Downloader {
public:
    static Downloader& Instance() {
        static Downloader instance;
        return instance;
    }

    void Push(std::string slug, std::vector<std::string> urls, bool urgent,
              std::string title = {}) {
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
                queue.push_front({std::move(slug), std::move(urls), std::move(title)});
            } else {
                // A full queue drops the request, the next game list refresh asks again.
                if (queue.size() >= MaxQueued || !seen.insert(slug).second) {
                    return;
                }
                queue.push_back({std::move(slug), std::move(urls), std::move(title)});
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
        std::string title;              ///< The name of the game, for the LaunchBox lookup
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

        // The repositories do not hold art for every game, so the LaunchBox Games Database is
        // asked for the cover of the game as well.
        if (!job.title.empty() && !stop_token.stop_requested()) {
            const auto game_page = LaunchBox::FindGamePage(job.title, stop_token);
            if (!game_page.empty()) {
                const auto box_url = LaunchBox::FindBoxUrl(game_page, stop_token);
                if (!box_url.empty()) {
                    std::string data;
                    const int status = Download(box_url, data, stop_token);
                    if (status == 200) {
                        if (!Store(path, data)) {
                            LOG_WARNING(Frontend, "Could not save box art to {}",
                                        Common::FS::PathToUTF8String(path));
                            return false;
                        }
                        LOG_INFO(Frontend, "Saved box art from {}", box_url);
                        Notify(job.slug);
                        return true;
                    }
                    last_status = status;
                    last_url = box_url;
                    all_missing = false;
                }
            }
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
    Downloader::Instance().Push(slug, std::move(urls), urgent, title);
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
