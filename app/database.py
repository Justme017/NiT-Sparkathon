import sqlite3
from datetime import datetime, date
from typing import List, Dict

DB_FILE = "telemetry.db"
EXAMPLE_DB = "db/example.db"

def get_all_telemetry() -> List[Dict]:
    conn = sqlite3.connect(DB_FILE)
    conn.row_factory = sqlite3.Row
    cursor = conn.cursor()
    cursor.execute("SELECT * FROM telemetry ORDER BY timestamp DESC")
    rows = [dict(row) for row in cursor.fetchall()]
    conn.close()
    return rows

def get_today_voice_classifications() -> List[Dict]:
    from db.voice import VoiceEntry
    
    conn = sqlite3.connect(EXAMPLE_DB)
    conn.row_factory = sqlite3.Row
    cursor = conn.cursor()
    
    today = date.today().strftime("%Y-%m-%d")

    cursor.execute("""
        SELECT * FROM voice 
        WHERE DATE(timestamp) = ? 
        ORDER BY timestamp DESC
    """, (today,))
    
    rows = [dict(row) for row in cursor.fetchall()]
    conn.close()
    
    voice_entries = []
    for row in rows:
        try:
            entry = VoiceEntry.from_row(row)
            voice_entries.append(entry)
        except Exception as e:
            print(f"Error parsing row: {e}")
            continue
    
    return voice_entries


class getData:
    def __init__(self, voice_entries):
        self.voice_entries = voice_entries
    
    def sum_speak_times(self) -> float:
        total_seconds = 0.0
        for entry in self.voice_entries:
            total_seconds += entry.duration.total_seconds()
        return total_seconds
    
    def sum_stressed_times(self) -> float:
        from db.voice import VoiceClassification
        
        stressed_seconds = 0.0
        for entry in self.voice_entries:
            if entry.classification == VoiceClassification.STRESSED:
                stressed_seconds += entry.duration.total_seconds()
        return stressed_seconds

