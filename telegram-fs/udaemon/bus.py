import os
import select
import time

from config import CHAT_MAP, REVERSE_MAP, BUS_MSG_MAX, log
from telegram import tg_send, tg_get_updates


def bus_write(bus_fd: int, line: str) -> None:
    """Write one protocol line to the bus device."""
    os.write(bus_fd, (line + "\n").encode())


def bus_inject_message(bus_fd: int, chat_idx: int,
                        sender: str, text: str) -> None:
    bus_write(bus_fd, f"IN|{chat_idx}|{sender}|{text}")


def bus_set_chat_id(bus_fd: int, chat_idx: int, tg_chat_id: int) -> None:
    bus_write(bus_fd, f"SET|{chat_idx}|{tg_chat_id}")


def handle_outgoing(bus_fd: int, line: str) -> None:
    """Parse OUT|<idx>|<message> and forward to Telegram."""
    parts = line.split("|", 2)
    if len(parts) != 3 or parts[0] != "OUT":
        log.warning("Malformed bus line: %r", line)
        return

    _, idx_str, message = parts

    try:
        chat_idx = int(idx_str)
    except ValueError:
        log.warning("Bad chat index: %r", idx_str)
        return

    tg_chat_id = CHAT_MAP.get(chat_idx)
    if tg_chat_id is None:
        log.warning("No Telegram chat mapped for index %d", chat_idx)
        return

    log.info("OUT chat_%d → TG %d: %r", chat_idx + 1, tg_chat_id, message)
    tg_send(tg_chat_id, message)


def outgoing_loop(bus_fd: int) -> None:
    """
    Blocking-read from /dev/telegram/bus.
    Each read() returns exactly one "OUT|<idx>|<msg>\n" line.
    """
    log.info("Outgoing thread started")

    while True:
        try:
            r, _, _ = select.select([bus_fd], [], [], 2.0)
            if not r:
                continue

            raw = os.read(bus_fd, BUS_MSG_MAX)
            if not raw:
                log.warning("Bus EOF — kernel module unloaded?")
                break

            line = raw.decode(errors="replace").strip()
            handle_outgoing(bus_fd, line)

        except OSError as e:
            log.error("Bus read error: %s", e)
            time.sleep(1)


def incoming_loop(bus_fd: int) -> None:
    """Long-poll Telegram and inject messages into kernel chat buffers."""
    log.info("Incoming thread started")
    offset = 0

    while True:
        for update in tg_get_updates(offset):
            offset = update["update_id"] + 1

            msg = update.get("message")
            if not msg or "text" not in msg:
                continue

            tg_chat_id = msg["chat"]["id"]
            sender     = msg["from"].get("first_name", "Unknown")
            text       = msg["text"]

            chat_idx = REVERSE_MAP.get(tg_chat_id)
            if chat_idx is None:
                log.debug("Ignored update from unmapped TG chat %d", tg_chat_id)
                continue

            log.info("IN TG %d → chat_%d [%s]: %r",
                     tg_chat_id, chat_idx + 1, sender, text)
            bus_inject_message(bus_fd, chat_idx, sender, text)
