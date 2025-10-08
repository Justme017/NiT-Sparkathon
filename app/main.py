from fastapi import FastAPI
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
from database import get_all_telemetry, getData
from test_voice import create_dummy_voice_entries

app = FastAPI()

app.mount("/static", StaticFiles(directory="static"), name="static")

@app.post("/api/telemetry")
async def telemetry(payload: dict):
    print("Telemetry:", payload)
    return {"ok": True}

@app.get("/api/data")
async def get_data():
    """API-Endpoint für JSON-Daten"""
    voice_entries = create_dummy_voice_entries()
    analyzer = getData(voice_entries)
    total_seconds = 2*analyzer.sum_speak_times()
    
    return {
        "total_speak_time_seconds": total_seconds,
        "total_speak_time_minutes": round(total_seconds / 60, 2),
        "number_of_entries": len(voice_entries)
    }

@app.get("/")
async def root():
    return FileResponse("static/index.html")
