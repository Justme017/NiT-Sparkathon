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

    def insert_query(self, table):
        """
        Get the insert query for this voice entry into the given table.
        """
        values = (self.timestamp.strftime("%Y-%m-%d %H:%M:%S"), self.classification.value,
                  int(self.duration.total_seconds() * 1000))
        return f"""
            INSERT INTO {table} (timestamp, classification, duration) VALUES (?, ?, ?)
        """, values

    def from_row(row):
        try:
            return VoiceEntry(
                datetime.strptime(row[1], "%Y-%m-%d %H:%M:%S"),
                VoiceClassification(row[2]),
                timedelta(milliseconds=row[3]),
            )
        except Exception:
            print("Couldnt parse db entry")

    def __str__(self):
        timestamp = self.timestamp.strftime("%Y-%m-%d %H:%M:%S")
        classification = self.classification.name
        ms = int(self.duration.total_seconds() * 1000)
        return f"At {timestamp}: {classification}, for {ms}ms"

    def __repr__(self):
        return self.__str__()
