# AntiGravity Chatbot v2.0

> *"Defying gravity, one personalised reply at a time."*

A modular, production-ready generative AI chatbot written entirely in **C11** — no Python, no external ML frameworks. Features natural conversation, sentiment awareness, persistent memory, a plugin system, an HTTP REST API, and the legendary **Antigravity Easter Egg** 🚀.

### New in v2.0: Persistent Memory & Personalization

- **Bot Naming**: Say *"call you HAL"* and the bot remembers its name across sessions.
- **Semantic Memory**: A custom vector store built completely from scratch using a 128-dimensional hashing-trick embedding and cosine-similarity top-k search.
- **Memory Injection**: Conversations are conditioned on past facts seamlessly retrieved from long-term memory.
- **Importance Decay**: Older or less relevant facts gradually fade away, keeping memory fast and relevant.
- **User Profiles**: Custom key-value store backing user preferences.

---

## Architecture Overview

```
User
 │
 ├──▶ [main.c] Naming Detector ("call you X") ──▶ [profile.c]
 │
 ├──▶ [embed.c] 128-dim Hash Embedding
 │      │
 │      ▼
 │    [semantic.c] Vector Search (Cosine Similarity)
 │      │
 │      ▼
 ├──▶ [model.c] NLP response (memory + persona injection)
 │      │
 │      ▼
 └──▶ [episodic.c] Sliding window dialogue context
```

## Module Summary

### Core & NLP

| File | Purpose |
|------|---------|
| `src/tokenizer.c` | Word tokenizer + vocabulary hash-map |
| `src/sentiment.c` | Keyword-based sentiment scoring |
| `src/model.c` | NLP engine: pattern matching + templates + memory |
| `src/main.c` | Entry point, pipeline orchestrator |

### Memory (`src/memory/`)

| File | Purpose |
|------|---------|
| `episodic.c` | Short-term sliding-window conversation history |
| `semantic.c` | Long-term vector store, cosine sim search |
| `profile.c` | Binary key-value store for user preferences |
| `embed.c` | 128-dim bag-of-words embedding generator |

### Extras

| File | Purpose |
|------|---------|
| `src/antigravity.c` | 🚀 Easter egg: lift force + ASCII art |
| `src/plugins.c` | Plugin system (`/help`, `/history`, etc.) |
| `src/server.c` | HTTP/1.1 server with thread pool |

---

## Building

### Prerequisites

- **Windows**: MinGW-w64 with `gcc` and `make` in `PATH`
- **Linux/macOS**: `gcc` and `make` (standard)

*(No external ML libraries or SQLite dependencies needed!)*

### Build the chatbot

```bash
make
```

### Run unit tests

```bash
make test
```

*(Runs all 6 test suites across Tokenizer, Memory, Sentiment, Antigravity, Semantic space, and Profile).*

---

## Usage

### CLI Mode (default)

```bash
chatbot.exe --cli
```

An interactive prompt appears. Try testing the new memory features:

```
  You: i'll call you HAL
  [Bot]: Understood! I'll go by HAL from now on.

  You: I really enjoy reading sci-fi novels.
  [HAL] (^_^): That's a great point! Tell me more about it.

  You: quit
```

Restart the bot:

```
  You: what is your name?
  [HAL]: My name is HAL! It's wonderful to chat with you.

  You: recommend a book genre
  [HAL] (-_-): (I remember: User said: I really enjoy reading sci-fi novels.) Hmm, let me think about that... Could you elaborate?
```

**Built-in commands:**

| Command | Description |
|---------|-------------|
| `/help` | Show available commands |
| `/history` | Show recent conversation turns |
| `/clear` | Clear session memory |
| `/version` | Show version info |
| `/lift` | Show current Antigravity lift force |
| `quit` | Exit the bot |

### Server Mode

```bash
chatbot.exe --server
```

Starts an HTTP server on port `8080`.

**Send a message:**

```bash
curl -X POST http://localhost:8080/chat \
     -H "Content-Type: application/json" \
     -d "{\"message\": \"hello\"}"
```

---

## The Antigravity Module 🚀

Trigger by saying any of:

- `antigravity` / `anti-gravity`
- `defy gravity` / `levitate`
- `float away` / `zero gravity`

**What happens:**

1. An ASCII animation plays (CLI mode)
2. Imaginary lift force is calculated:
   `F_lift = 70 kg × 9.81 m/s² × (1 + sentiment_score / 10)`

---

## License

MIT — Build, learn, and remember! (^o^)/
