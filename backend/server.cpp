/*
 * server.cpp  –  Autocomplete Search Engine Backend
 *
 * Uses cpp-httplib (single-header, no install needed) to expose:
 *   GET /suggest?prefix=   → top-5 suggestions
 *   GET /search?word=      → exact match + frequency
 *   GET /correct?word=     → fuzzy corrections (edit distance ≤ 2)
 *
 * Compile (MinGW / MSVC):
 *   g++ -std=c++17 -O2 -o server.exe server.cpp -lws2_32
 */
#define _WIN32_WINNT 0x0A00
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

// ──────────────────────────────────────────────
//  cpp-httplib  (place httplib.h next to this file)
//  Download: https://github.com/yhirose/cpp-httplib
// ──────────────────────────────────────────────
#include "httplib.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT 0

using namespace std;

// ============================================================
//  Trie  (your original logic – untouched)
// ============================================================

class TrieNode {
public:
    TrieNode* children[26];
    bool isEnd;
    int frequency;

    TrieNode() {
        for (int i = 0; i < 26; i++) children[i] = nullptr;
        isEnd = false;
        frequency = 0;
    }
};

class Trie {
private:
    TrieNode* root;

    string toLowerCase(string word) {
        for (char& c : word) c = tolower(c);
        return word;
    }

    void collectAllWords(TrieNode* node, string cur, vector<pair<string,int>>& res) {
        if (!node) return;
        if (node->isEnd) res.push_back({cur, node->frequency});
        for (int i = 0; i < 26; i++)
            if (node->children[i])
                collectAllWords(node->children[i], cur + char('a'+i), res);
    }

    void deleteAll(TrieNode* node) {
        if (!node) return;
        for (int i = 0; i < 26; i++) deleteAll(node->children[i]);
        delete node;
    }

    int editDistance(const string& a, const string& b) {
        int m = a.size(), n = b.size();
        vector<vector<int>> dp(m+1, vector<int>(n+1));
        for (int i = 0; i <= m; i++) dp[i][0] = i;
        for (int j = 0; j <= n; j++) dp[0][j] = j;
        for (int i = 1; i <= m; i++)
            for (int j = 1; j <= n; j++)
                dp[i][j] = (a[i-1]==b[j-1]) ? dp[i-1][j-1]
                          : 1 + min({dp[i-1][j-1], dp[i-1][j], dp[i][j-1]});
        return dp[m][n];
    }

public:
    Trie()  { root = new TrieNode(); }
    ~Trie() { deleteAll(root); }

    void insertWord(string word, int freq = 1) {
        word = toLowerCase(word);
        TrieNode* cur = root;
        for (char c : word) {
            int idx = c - 'a';
            if (!cur->children[idx]) cur->children[idx] = new TrieNode();
            cur = cur->children[idx];
        }
        cur->isEnd = true;
        cur->frequency += freq;
    }

    bool searchWord(string word) {
        word = toLowerCase(word);
        TrieNode* cur = root;
        for (char c : word) {
            int idx = c - 'a';
            if (!cur->children[idx]) return false;
            cur = cur->children[idx];
        }
        return cur->isEnd;
    }

    int getFrequency(string word) {
        word = toLowerCase(word);
        TrieNode* cur = root;
        for (char c : word) {
            int idx = c - 'a';
            if (!cur->children[idx]) return -1;
            cur = cur->children[idx];
        }
        return cur->isEnd ? cur->frequency : -1;
    }

    void recordSearch(string word) {
        word = toLowerCase(word);
        TrieNode* cur = root;
        for (char c : word) {
            int idx = c - 'a';
            if (!cur->children[idx]) return;
            cur = cur->children[idx];
        }
        if (cur->isEnd) cur->frequency++;
    }

    vector<pair<string,int>> getSuggestions(string prefix) {
        prefix = toLowerCase(prefix);
        TrieNode* cur = root;
        vector<pair<string,int>> res;
        for (char c : prefix) {
            int idx = c - 'a';
            if (!cur->children[idx]) return res;
            cur = cur->children[idx];
        }
        collectAllWords(cur, prefix, res);
        return res;
    }

    vector<pair<string,int>> getTopK(string prefix, int k = 5) {
        auto res = getSuggestions(prefix);
        sort(res.begin(), res.end(), [](auto& a, auto& b){ return a.second > b.second; });
        if ((int)res.size() > k) res.resize(k);
        return res;
    }

    vector<pair<string,int>> fuzzyCorrect(string typo, int maxDist = 2) {
        typo = toLowerCase(typo);
        vector<pair<string,int>> all, corrections;
        collectAllWords(root, "", all);
        for (auto& [word, freq] : all) {
            int d = editDistance(typo, word);
            if (d > 0 && d <= maxDist) corrections.push_back({word, freq});
        }
        sort(corrections.begin(), corrections.end(), [](auto& a, auto& b){ return a.second > b.second; });
        if ((int)corrections.size() > 5) corrections.resize(5);
        return corrections;
    }

    int loadData(const string& path) {
        ifstream file(path);
        if (!file.is_open()) return -1;
        string word; int count = 0;
        while (getline(file, word)) {
            while (!word.empty() && (word.back()=='\r'||word.back()==' ')) word.pop_back();
            if (!word.empty()) { insertWord(word, 1); count++; }
        }
        return count;
    }
};

// ============================================================
//  Simple JSON helpers  (no external library needed)
// ============================================================

// Escape a string for JSON
string jsonStr(const string& s) {
    string out = "\"";
    for (char c : s) {
        if (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else out += c;
    }
    out += '"';
    return out;
}

// Build a JSON array of {word, frequency} pairs
string pairsToJson(const vector<pair<string,int>>& v) {
    string out = "[";
    for (int i = 0; i < (int)v.size(); i++) {
        if (i) out += ",";
        out += "{\"word\":" + jsonStr(v[i].first)
             + ",\"frequency\":" + to_string(v[i].second) + "}";
    }
    out += "]";
    return out;
}

// ============================================================
//  main – mount routes and start server
// ============================================================

int main() {
    Trie trie;

    cout << "Loading words...\n";
    int loaded = trie.loadData("5000-more-common.txt");
    if (loaded < 0) {
        cerr << "ERROR: Could not open '5000-more-common.txt'.\n"
             << "Make sure it's in the same folder as server.exe\n";
        return 1;
    }
    cout << loaded << " words loaded.\n";

    httplib::Server svr;

    // ── CORS pre-flight ──────────────────────────────────────
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin",  "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 204;
    });

    auto addCors = [](httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
    };

    // ── GET /suggest?prefix=<text> ───────────────────────────
    // Returns top-5 suggestions sorted by frequency.
    // Example: /suggest?prefix=hel
    // → [{"word":"hello","frequency":3},{"word":"help","frequency":1},...]
    svr.Get("/suggest", [&](const httplib::Request& req, httplib::Response& res) {
        addCors(res);
        string prefix = req.has_param("prefix") ? req.get_param_value("prefix") : "";
        auto results  = trie.getTopK(prefix, 5);
        string body   = "{\"prefix\":" + jsonStr(prefix)
                      + ",\"suggestions\":" + pairsToJson(results) + "}";
        res.set_content(body, "application/json");
    });

    // ── GET /search?word=<text> ──────────────────────────────
    // Returns whether the word exists and its search frequency.
    // Example: /search?word=hello
    // → {"word":"hello","found":true,"frequency":3}
    svr.Get("/search", [&](const httplib::Request& req, httplib::Response& res) {
        addCors(res);
        string word  = req.has_param("word") ? req.get_param_value("word") : "";
        bool   found = trie.searchWord(word);
        int    freq  = found ? trie.getFrequency(word) : 0;
        if (found) trie.recordSearch(word);   // bump frequency on every search
        string body  = "{\"word\":" + jsonStr(word)
                     + ",\"found\":"     + (found ? "true" : "false")
                     + ",\"frequency\":" + to_string(freq) + "}";
        res.set_content(body, "application/json");
    });

    // ── GET /correct?word=<text> ─────────────────────────────
    // Returns fuzzy-corrected suggestions (edit distance ≤ 2).
    // Example: /correct?word=helo
    // → {"typo":"helo","corrections":[{"word":"hello","frequency":3},...]}
    svr.Get("/correct", [&](const httplib::Request& req, httplib::Response& res) {
        addCors(res);
        string word    = req.has_param("word") ? req.get_param_value("word") : "";
        auto   results = trie.fuzzyCorrect(word);
        string body    = "{\"typo\":"        + jsonStr(word)
                       + ",\"corrections\":" + pairsToJson(results) + "}";
        res.set_content(body, "application/json");
    });

    // ── POST /insert ─────────────────────────────────────────
    // Inserts a new word into the Trie.
    // Example POST body: {"word":"newword","frequency":5}
    // → {"success":true,"word":"newword","message":"Word inserted successfully"}
    svr.Post("/insert", [&](const httplib::Request& req, httplib::Response& res) {
        addCors(res);
        string body = req.body;
        
        // Simple JSON parsing for {"word":"...","frequency":...}
        size_t wordPos = body.find("\"word\"");
        size_t freqPos = body.find("\"frequency\"");
        
        string word = "";
        int frequency = 1;
        
        if (wordPos != string::npos) {
            size_t colonPos = body.find(":", wordPos);
            size_t quoteStart = body.find("\"", colonPos);
            size_t quoteEnd = body.find("\"", quoteStart + 1);
            if (quoteStart != string::npos && quoteEnd != string::npos) {
                word = body.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
            }
        }
        
        if (freqPos != string::npos) {
            size_t colonPos = body.find(":", freqPos);
            size_t numStart = colonPos + 1;
            while (numStart < body.size() && !isdigit(body[numStart])) numStart++;
            size_t numEnd = numStart;
            while (numEnd < body.size() && isdigit(body[numEnd])) numEnd++;
            if (numStart < body.size()) {
                frequency = stoi(body.substr(numStart, numEnd - numStart));
            }
        }
        
        if (word.empty()) {
            string response = "{\"success\":false,\"message\":\"No word provided\"}";
            res.set_content(response, "application/json");
            return;
        }
        
        trie.insertWord(word, frequency);
        
        string response = "{\"success\":true,\"word\":" + jsonStr(word)
                        + ",\"frequency\":" + to_string(frequency)
                        + ",\"message\":\"Word inserted successfully\"}";
        res.set_content(response, "application/json");
    });

    cout << "Server running at http://localhost:8080\n";
    cout << "Press Ctrl+C to stop.\n";
    svr.listen("0.0.0.0", 8080);
    return 0;
}
