import os
import threading

from config import BUS_DEVICE, CHAT_MAP, log
from bus import bus_set_chat_id, outgoing_loop, incoming_loop


def main() -> None:
    log.info("Opening %s", BUS_DEVICE)
    bus_fd = os.open(BUS_DEVICE, os.O_RDWR)

    for chat_idx, tg_chat_id in CHAT_MAP.items():
        bus_set_chat_id(bus_fd, chat_idx, tg_chat_id)
        log.info("Mapped chat_%d ↔ Telegram chat %d",
                 chat_idx + 1, tg_chat_id)

    t = threading.Thread(target=incoming_loop, args=(bus_fd,),
                         name="incoming", daemon=True)
    t.start()

    outgoing_loop(bus_fd)

    os.close(bus_fd)


if __name__ == "__main__":
    main()
