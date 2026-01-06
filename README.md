# NiT Sparkathon 2025

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)

**Team repository for the NiT Sparkathon, a 2-day hardware hackathon focused on solving real-world problems with physical prototypes. Hosted at the Northern Institute of Technology Management in Hamburg.**

---

## About the Event

The **NiT Sparkathon** is an invention marathon where innovators, students, and founders come together to build tangible solutions to real-world challenges. Over two intensive days, teams will ideate, design, and build a working hardware prototype from the ground up.

Our mission is to bridge the gap between idea and impact. We provide the hardware, mentorship, and a creative environment; you bring the vision and the drive to build the future.

* **What:** A 2-day hardware hackathon.
* **Where:** Room E08, Northern Institute of Technology Management (NiT), Hamburg.
* **When:** October 8-9, 2025.
* **Theme:** Solving real-world problems with physical prototypes.

---

## NiT Sparkathon 2025 — Team:  Alpha

Welcome! This fork contains our hackathon project built during the NiT Sparkathon (Oct 8–9, 2025).

Project: Voice Stress Monitor

Short description
 - A lightweight prototype that captures short voice recordings, classifies them as STRESSED or CALM, stores telemetry in a local SQLite database, and exposes a small FastAPI service for retrieving daily "calmness" scores and receiving live status updates from devices.

Why this project
 - Stress detection from short voice samples can help monitor well-being in teams and workplaces. Our prototype focuses on usability: low-power recording, simple classification, and a clear daily score aggregation.

Repository layout (important files)
 - `app/main.py` — FastAPI application and HTTP endpoints (/api/data, /api/telemetry, /api/send_status).
 - `app/database.py` — helpers for reading telemetry and computing aggregated metrics from voice entries.
 - `app/test_voice.py` — dummy data generator used during development.
 - `app/db/voice.py` — `VoiceEntry` model and `VoiceClassification` enum + serialization helpers.
 - `app/db/handler.py` — SQLite helper functions, DB initialization, insert helpers, and daily score calculation.
 - `db/example.db` — example SQLite database (committed sample data).
 - `static/index.html` — small frontend (served at `/`) that demonstrates the project.

Quick start (local, development)
1. Create a Python venv and install dependencies (Windows PowerShell):

	python -m venv .venv; .\.venv\Scripts\Activate.ps1; pip install -r requirements.txt

2. Start the API (development):

	uvicorn app.main:app --reload

3. Open your browser at http://127.0.0.1:8000/ to view the demo UI or call `/api/data` to get today's score.

4. Flash firmware in "Hardware Code" to ESP32 using PlatformIO or Arduino IDE.

API Endpoints
 - GET /api/data?offset={int} — returns JSON with a `score` key (percentage calm for the requested day; offset in days from today).
 - POST /api/send_status — accepts JSON {"status": <1|2>} where 1 = STRESSED, 2 = CALM; converts to a `VoiceEntry` and stores it in the local DB.
 - GET / — serves `static/index.html` demo frontend.

Database and data model
 - The app uses a local SQLite database file `data.db` (created by `app/db/handler.py`).
 - Table `voice` schema: (id INTEGER PK, timestamp TEXT, classification INTEGER, duration INTEGER)
 - `VoiceClassification` enum values: STRESSED=1, CALM=2
 - `VoiceEntry` stores timestamp (datetime), classification (enum), and duration (timedelta). `handler.percentage_for_day(day)` returns the percentage of CALM entries on the passed day.

Notes for maintainers and judges
 - The repository is deliberately small and focused. The classification in this prototype is placeholder logic; integrate a proper model or on-device inference for production use.
 - Example DB: `db/example.db` can be used to inspect sample entries.

Licensing and attribution
 - This project is based on the NiT Sparkathon template. Any third-party libraries used are listed in `requirements.txt`.

Contact
 - Team Alpha — contact via the GitHub repo for follow-ups.

---

Thank you and good luck to all teams!

```
