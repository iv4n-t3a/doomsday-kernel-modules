import logging

BOT_TOKEN  = "Your bot token"
BUS_DEVICE = "/dev/telegram/bus"
API_BASE   = f"https://api.telegram.org/bot{BOT_TOKEN}"

# kernel chat_idx (0-based) → Telegram chat_id
CHAT_MAP: dict[int, int] = {
    0: 1, # /dev/telegram/chat_1
    1: 2, # /dev/telegram/chat_2
}
REVERSE_MAP: dict[int, int] = {v: k for k, v in CHAT_MAP.items()}

BUS_MSG_MAX = 512

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(threadName)s] %(levelname)s %(message)s",
)
log = logging.getLogger("tgfs")
