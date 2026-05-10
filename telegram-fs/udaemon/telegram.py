import time
import requests

from config import API_BASE, log


def tg_send(chat_id: int, text: str) -> bool:
    try:
        r = requests.post(f"{API_BASE}/sendMessage",
                          json={"chat_id": chat_id, "text": text},
                          timeout=10)
        r.raise_for_status()
        return True
    except Exception as e:
        log.error("sendMessage failed: %s", e)
        return False


def tg_get_updates(offset: int) -> list[dict]:
    try:
        r = requests.get(f"{API_BASE}/getUpdates",
                         params={"offset": offset,
                                 "timeout": 30,
                                 "allowed_updates": ["message"]},
                         timeout=40)
        r.raise_for_status()
        data = r.json()
        if data.get("ok"):
            return data["result"]
    except requests.Timeout:
        pass
    except Exception as e:
        log.error("getUpdates failed: %s", e)
        time.sleep(5)
    return []
