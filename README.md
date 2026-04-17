# 🔍 AutoComplete Search Engine using Trie Data Structure

A full-stack autocomplete search engine powered by a **Trie (Prefix Tree)** data structure. The backend is built in **C++** with a lightweight HTTP server, and the frontend is a modern single-page web application with real-time suggestions and fuzzy spell correction.

---

## 👨‍💻 Team Members

| Name | Role |
|------|------|
| **Shivom Singh** | Backend Lead — Trie design, HTTP server, JSON API |
| **Aditya Prakash** | Algorithm Specialist — Edit distance DP, fuzzy correction, Top-K sorting |
| **Shelar Yash Kanifnath** | Frontend Engineer — UI/UX, glassmorphism design, real-time autocomplete |
| **Tumma Abhinav** | Integration & QA — API wiring, CORS, corpus loading, testing |

---

## 📋 Table of Contents

- [Features](#features)
- [Project Structure](#project-structure)
- [Getting Started](#getting-started)
- [API Reference](#api-reference)
- [How It Works](#how-it-works)
- [Tech Stack](#tech-stack)
- [Complexity Analysis](#complexity-analysis)
- [Future Scope](#future-scope)

---

## ✨ Features

- ⚡ **Real-time Autocomplete** — Top-5 suggestions ranked by frequency as you type
- 🔎 **Exact Word Search** — Lookup with frequency tracking (every search bumps the counter)
- 🧠 **Fuzzy Spell Correction** — Levenshtein edit distance ≤ 2 to catch typos
- ➕ **Dynamic Insertion** — Add new words with custom frequencies at runtime via POST API
- 📊 **Frequency-Aware Ranking** — Popular words always surface first
- 🌐 **REST API with CORS** — Accessible from any browser or HTTP client
- 🎨 **Modern Dark UI** — Glassmorphism design with purple gradient theme

---

## 📁 Project Structure

```
AutoComplete-Search-Engine/
├── server.cpp               # C++ backend — Trie + HTTP routes
├── httplib.h                # Single-header HTTP library (cpp-httplib)
├── server.exe               # Pre-compiled Windows executable
├── index.html               # Frontend SPA (HTML + CSS + JS)
├── 5000-more-common.txt     # Word corpus — one word per line
└── README.md
```

---

## 🚀 Getting Started

### Prerequisites

- Windows OS (for the pre-built `server.exe`)
- A modern web browser (Chrome, Firefox, Edge)
- *(Optional for recompiling)*: MinGW / MSVC with C++17 support

### Running the Server

1. Place all files in the **same directory**.
2. Double-click `server.exe` **or** run from terminal:

```bash
./server.exe
```

3. You should see:
```
Loading words...
5000 words loaded.
Server running at http://localhost:8080
Press Ctrl+C to stop.
```

### Opening the Frontend

Simply open `index.html` in your browser. The UI will automatically connect to `http://localhost:8080`.

> ⚠️ The server must be running before opening the frontend.

### Recompiling from Source

```bash
g++ -std=c++17 -O2 -o server.exe server.cpp -lws2_32
```

---

## 📡 API Reference

All endpoints are served at `http://localhost:8080`. Responses are `application/json`.

### `GET /suggest?prefix=<text>`

Returns top-5 word suggestions for the given prefix, sorted by frequency.

```bash
GET /suggest?prefix=hel
```

```json
{
  "prefix": "hel",
  "suggestions": [
    { "word": "hello", "frequency": 3 },
    { "word": "help",  "frequency": 1 }
  ]
}
```

---

### `GET /search?word=<text>`

Exact dictionary lookup. Increments the word's frequency on every successful search.

```bash
GET /search?word=hello
```

```json
{
  "word": "hello",
  "found": true,
  "frequency": 4
}
```

---

### `GET /correct?word=<text>`

Returns up to 5 fuzzy-corrected suggestions for misspelled input (edit distance ≤ 2).

```bash
GET /correct?word=helo
```

```json
{
  "typo": "helo",
  "corrections": [
    { "word": "hello", "frequency": 3 }
  ]
}
```

---

### `POST /insert`

Inserts a new word into the Trie at runtime with an optional frequency.

```bash
POST /insert
Content-Type: application/json

{ "word": "quantum", "frequency": 5 }
```

```json
{
  "success": true,
  "word": "quantum",
  "frequency": 5,
  "message": "Word inserted successfully"
}
```

---

## ⚙️ How It Works

### Trie Data Structure

A **Trie** (Prefix Tree) stores strings by sharing common prefixes. Each node has:
- `children[26]` — pointers to child nodes (one per letter a–z)
- `isEnd` — marks the end of a valid word
- `frequency` — tracks how often the word has been searched

```
          root
         /    \
        h      w
        |      |
        e      o
        |      |
        l    [word]
       / \
      l   p
      |   |
    [end] [end]
   "hell" "help"
```

### Autocomplete Flow

1. Navigate the Trie to the node matching the last character of the prefix
2. DFS from that node to collect all words in the subtree
3. Sort by frequency descending, return top K

### Fuzzy Correction

Uses the classic **Levenshtein edit distance** dynamic programming algorithm. For a query of length M and a dictionary word of length N:

- Time: O(M × N) per word
- Space: O(M × N) per comparison

Only words with `0 < distance ≤ 2` are returned (distance 0 = exact match, handled by `/search`).

---

## 🛠️ Tech Stack

| Layer | Technology |
|-------|-----------|
| Data Structure | Custom Trie (C++) |
| Backend Language | C++17 |
| HTTP Server | cpp-httplib (single-header) |
| Serialization | Hand-rolled JSON (no libs) |
| Frontend | Vanilla HTML5 / CSS3 / JavaScript |
| API Communication | Fetch API |
| Styling | Glassmorphism + CSS Grid/Flexbox |

---

## 📊 Complexity Analysis

| Operation | Time Complexity | Space Complexity |
|-----------|----------------|-----------------|
| Insert word | O(L) | O(L × 26) |
| Search word | O(L) | O(1) extra |
| Prefix suggestions | O(L + N) | O(N) output |
| Top-K suggestions | O(L + N log N) | O(N) |
| Fuzzy correction | O(W × L²) | O(L²) per word |

*L = word/prefix length, N = matching words, W = dictionary size*

---

## 🔮 Future Scope

- [ ] **Persistent storage** — Serialize Trie to disk (JSON/binary) across restarts
- [ ] **Delete operation** — Word removal with Trie pruning
- [ ] **Weighted corpus** — Pre-assign frequencies from a real search log
- [ ] **Ranked fuzzy matching** — Score by both frequency and edit distance
- [ ] **Multi-language support** — Unicode character sets beyond a–z
- [ ] **HTTPS & auth** — Secure the API for production deployment
- [ ] **Docker support** — Containerize for cross-platform deployment

---

## 📄 License

This project was developed as an academic submission for a Data Structures & Algorithms course.

---

*Built with ❤️ by Shivom Singh, Aditya Prakash, Shelar Yash Kanifnath & Tumma Abhinav*
