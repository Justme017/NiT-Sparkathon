from datetime import datetime, timedelta
from db.voice import VoiceEntry, VoiceClassification

def create_dummy_voice_entries():
    """Erstellt eine Dummy-Liste von VoiceEntry-Objekten"""
    
    dummy_entries = [
        VoiceEntry(
            timestamp=datetime(2024, 10, 8, 10, 30, 0),
            classification=VoiceClassification.STRESSED,
            duration=timedelta(seconds=1.2)
        ),
        VoiceEntry(
            timestamp=datetime(2024, 10, 8, 10, 31, 15),
            classification=VoiceClassification.CALM,
            duration=timedelta(seconds=2.7)
        ),
        VoiceEntry(
            timestamp=datetime(2024, 10, 8, 10, 32, 30),
            classification=VoiceClassification.STRESSED,
            duration=timedelta(seconds=0.8)
        ),
        VoiceEntry(
            timestamp=datetime(2024, 10, 8, 10, 33, 45),
            classification=VoiceClassification.CALM,
            duration=timedelta(seconds=3.5)
        ),
        VoiceEntry(
            timestamp=datetime(2024, 10, 8, 10, 35, 0),
            classification=VoiceClassification.STRESSED,
            duration=timedelta(seconds=1.9)
        ),
    ]
    
    return dummy_entries

def main():
    entries = create_dummy_voice_entries()
    
    print(f"Anzahl Dummy-Einträge: {len(entries)}\n")
    
    for i, entry in enumerate(entries, 1):
        print(f"Entry {i}:")
        print(f"  Timestamp: {entry.timestamp}")
        print(f"  Classification: {entry.classification.name}")
        print(f"  Duration: {entry.duration.total_seconds()}s")
        print()

if __name__ == "__main__":
    main()
