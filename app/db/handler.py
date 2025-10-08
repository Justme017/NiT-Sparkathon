import sqlite3
from voice import VoiceClassification, VoiceEntry
from datetime import timedelta, datetime
import random


def execute_query(db_path, query, params=None):
    """
    Execute a SQL query on a local SQLite database.

    Args:
        db_path (str): Path to the SQLite database file.
        query (str): SQL query to execute.
        params (tuple, optional): Parameters to safely substitute into the query.

    Returns:
        list: Query results (for SELECT statements), or an empty list for others.
    """
    try:
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()

        if params:
            cursor.execute(query, params)
        else:
            cursor.execute(query)

        if query.strip().lower().startswith(("insert", "update", "delete", "create", "drop")):
            conn.commit()
            result = []
        else:
            result = cursor.fetchall()

        return result

    except sqlite3.Error as e:
        print(f"SQLite error: {e}")
        return []
    finally:
        if conn:
            conn.close()


dummy_entries = []

# Assume recordings happen roughly every 1-2 hours during a 16-hour day
start_time = datetime(2025, 10, 8, 6, 0)  # Start at 6 AM
for i in range(10):  # 10 recordings in a day
    # slight random offset
    timestamp = start_time + timedelta(hours=i*1.5 + random.uniform(-0.2, 0.2))
    classification = random.choice(
        [VoiceClassification.STRESSED, VoiceClassification.CALM])
    # duration between 30s and 5min
    duration = timedelta(seconds=random.randint(30, 300))
    dummy_entries.append(VoiceEntry(timestamp, classification, duration))


# Example usage
if __name__ == "__main__":
    db_file = "example.db"
    table = "voice"

    # Clean
    execute_query(db_file, f"DROP TABLE {table}")

    # Setup
    execute_query(db_file, f"""
        CREATE TABLE IF NOT EXISTS {table} (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT,
            classification INTEGER,
            duration INTEGER
        )
    """)

    # Add dummy data
    for entry in dummy_entries:
        query = entry.insert_query(table)
        execute_query(db_file, query[0], query[1])

    rows = execute_query(db_file, f"SELECT * FROM {table}")
    for row in rows:
        print(VoiceEntry.from_row(row))
