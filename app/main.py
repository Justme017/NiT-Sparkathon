from test_voice import create_dummy_voice_entries
from db.voice import VoiceClassification, VoiceEntry
import db.handler as handler
from database import get_all_telemetry, getData
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
from fastapi import FastAPI, Request
from datetime import datetime, timedelta

app = FastAPI()

app.mount("/static", StaticFiles(directory="static"), name="static")


@app.post("/api/telemetry")
async def telemetry(payload: dict):
    print("Telemetry:", payload)
    return {"ok": True}


@app.get("/api/data")
async def get_data(offset: int | None = None):
    """API-Endpoint für JSON-Daten"""
    # voice_entries = create_dummy_voice_entries()
    # analyzer = getData(voice_entries)
    # total_seconds = 2*analyzer.sum_speak_times()

    # TODO: add date argument to endpoint
    target_date = datetime.now().date()

    if offset is not None:
        target_date += timedelta(days=offset)

    score = handler.percentage_for_day(target_date)
    print(f"Score is {score}")
    return {
        "score": score,
    }


@app.post("/api/send_status")
async def receive_status(request: Request):
    data = await request.json()
    if "status" not in data:
        print("Received invalid status information")
        return
    status = VoiceClassification(data["status"])
    time = datetime.now()
    delta = timedelta(microseconds=1)
    entry = VoiceEntry(time, status, delta)
    handler.insert(entry)
    print("Inserted entry: ", entry.classification.name)
    return {"message": "JSON received successfully"}


@app.get("/")
async def root():
    return FileResponse("static/index.html")


@app.on_event("startup")
async def startup_event():
    print("App is starting up!")
    handler.init()
    print("DB is available")
