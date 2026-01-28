// Minimal search + chat agent using local Ollama.
// Dependencies: libcurl, nlohmann_json.

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace {

const std::string kDefaultModel = "phi4";
const std::string kDefaultOllamaUrl = "http://localhost:11434/api/chat";
const std::string kUserAgent = "search-agent-cpp/0.1";

size_t WriteToString(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    auto* s = static_cast<std::string*>(userp);
    s->append(static_cast<char*>(contents), total);
    return total;
}

std::string urlEncode(CURL* curl, const std::string& s) {
    char* out = curl_easy_escape(curl, s.c_str(), static_cast<int>(s.size()));
    if (!out) return "";
    std::string encoded(out);
    curl_free(out);
    return encoded;
}

bool isOffline();  // forward declaration

std::string httpGet(const std::string& url_with_query) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl init failed");

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url_with_query.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, kUserAgent.c_str());
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 8L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::string err = curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        throw std::runtime_error("GET failed: " + err);
    }
    curl_easy_cleanup(curl);
    return response;
}

std::string httpPostJson(const std::string& url, const json& body) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl init failed");

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("User-Agent: " + kUserAgent).c_str());

    std::string response;
    auto body_str = body.dump();
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_str.c_str());
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 8L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    if (res != CURLE_OK) {
        std::string err = curl_easy_strerror(res);
        curl_easy_cleanup(curl);
        throw std::runtime_error("POST failed: " + err);
    }
    curl_easy_cleanup(curl);
    return response;
}

std::vector<json> wikiSearch(CURL* curl, const std::string& query, int limit = 3, const std::string& lang = "en") {
    std::stringstream ss;
    ss << "https://" << lang << ".wikipedia.org/w/api.php?action=query&list=search&format=json";
    ss << "&srlimit=" << limit;
    ss << "&srsearch=" << urlEncode(curl, query);
    auto resp = httpGet(ss.str());
    json doc = json::parse(resp, nullptr, false);
    std::vector<json> out;
    if (!doc.is_object()) return out;
    for (auto& item : doc.value("query", json::object()).value("search", json::array())) {
        out.push_back({
            {"source", "wikipedia"},
            {"title", item.value("title", "")},
            {"snippet", item.value("snippet", "")},
        });
    }
    return out;
}

std::string stripTags(const std::string& html) {
    return std::regex_replace(html, std::regex("<[^>]*>"), "");
}

std::vector<json> ddgSearch(const std::string& query, int limit = 3) {
    // Simple HTML scrape; not robust but sufficient for demo.
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl init failed");
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, "https://duckduckgo.com/html/");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, kUserAgent.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    std::string postfields = "q=" + query;
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields.c_str());
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 8L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) {
        return {};
    }

    std::vector<json> results;
    std::regex link_re(
        R"REG(<a[^>]*class="result__a"[^>]*href="([^"]+)"[^>]*>(.*?)</a>)REG",
        std::regex::icase);
    auto begin = std::sregex_iterator(response.begin(), response.end(), link_re);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end && static_cast<int>(results.size()) < limit; ++it) {
        std::string href = (*it)[1].str();
        std::string title = stripTags((*it)[2].str());
        results.push_back({{"source", "duckduckgo"}, {"title", title}, {"link", href}});
    }
    return results;
}

std::vector<json> webSearch(const std::string& query, int top_k = 3) {
    if (isOffline()) {
        return {};
    }
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl init failed");
    std::vector<json> out;
    try {
        auto w = wikiSearch(curl, query, top_k);
        out.insert(out.end(), w.begin(), w.end());
    } catch (...) {
        std::cerr << "[warn] wikipedia search failed\n";
    }
    curl_easy_cleanup(curl);
    if (static_cast<int>(out.size()) < top_k) {
        try {
            auto d = ddgSearch(query, top_k - static_cast<int>(out.size()));
            out.insert(out.end(), d.begin(), d.end());
        } catch (...) {
            std::cerr << "[warn] duckduckgo search failed\n";
        }
    }
    if (static_cast<int>(out.size()) > top_k) out.resize(static_cast<size_t>(top_k));
    return out;
}

std::string getEnvOr(const char* key, const std::string& def) {
    const char* v = std::getenv(key);
    return v ? std::string(v) : def;
}

bool isOffline() {
    const char* v = std::getenv("OFFLINE_MODE");
    // Default to offline (no web search). Set OFFLINE_MODE=0/false/no/off to enable web.
    if (!v) return true;
    std::string s(v);
    for (auto& c : s) c = static_cast<char>(std::tolower(c));
    if (s == "0" || s == "false" || s == "no" || s == "off") return false;
    return true;
}

std::string ollamaChat(const std::string& url, const std::string& model, const std::vector<json>& messages) {
    json payload = {{"model", model}, {"messages", messages}, {"stream", false}};
    auto resp = httpPostJson(url, payload);
    json doc = json::parse(resp, nullptr, false);
    if (!doc.is_object()) return "invalid model response";
    auto msg = doc.value("message", json::object());
    return msg.value("content", std::string("no response from model"));
}

void loop() {
    std::string model = getEnvOr("OLLAMA_MODEL", kDefaultModel);
    std::string url = getEnvOr("OLLAMA_URL", kDefaultOllamaUrl);
    bool offline = isOffline();

    std::vector<json> history = {
        {{"role", "system"}, {"content", "You are a concise assistant. Use provided search results to answer."}}};

    std::cout << "Type your question (exit to quit)." << std::endl;
    std::cout << "Using model=" << model << " endpoint=" << url << (offline ? " (offline mode: no web search)" : "")
              << std::endl;
    while (true) {
        std::cout << "You: ";
        std::string line;
        if (!std::getline(std::cin, line)) break;
        if (line == "exit" || line == "quit") break;

        auto results = webSearch(line, 3);
        std::string ctx = "Search results: " + json(results).dump();
        history.push_back({{"role", "user"}, {"content", line}});
        history.push_back({{"role", "system"}, {"content", ctx}});

        try {
            std::cout << "Bot: (thinking...)\n";
            std::string reply = ollamaChat(url, model, history);
            std::cout << "Bot: " << reply << "\n";
            history.push_back({{"role", "assistant"}, {"content", reply}});
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
    }
}

}  // namespace

int main() {
    try {
        loop();
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
