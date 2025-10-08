from __future__ import annotations
from datetime import datetime, timedelta
from enum import Enum


class VoiceClassification(Enum):
    STRESSED = 1
    CALM = 2


class VoiceEntry:
    def __init__(self,
                 timestamp: datetime,
                 classification: VoiceClassification,
                 duration: timedelta):
        self.timestamp = timestamp
        self.classification = classification
        self.duration = duration

    def from_query():
        pass

    def from_row(row):
        try:
            return VoiceEntry(
                datetime.strptime(row[1], "%Y-%m-%d %H:%M:%S"),
                VoiceClassification(row[2]),
                timedelta(milliseconds=row[3]),
            )
        except Exception:
            print("Couldnt parse db entry")
