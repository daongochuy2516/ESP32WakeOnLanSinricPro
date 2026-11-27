"""Send a signed SinricPro local-control (LAN/UDP) command to the device.

Mirrors the SinricPro 5.x SDK: the device listens on UDP 3333, verifies
HMAC-SHA256(APP_SECRET, <payload JSON as transmitted>) and replies over UDP.

Settings are read from the project's .env (see .env.example):
  SINRIC_DEVICE_IP, SINRIC_DEVICE_ID, SINRIC_APP_SECRET

Usage:
  python tools/test.py [on|off] [--bad-signature]
  python tools/test.py on --ip 192.168.1.50   # override a .env value
"""

import argparse
import base64
import hashlib
import hmac
import json
import os
import socket
import sys
import time
import uuid
from pathlib import Path

PORT = 3333
PROJECT_DIR = Path(__file__).resolve().parent.parent


def load_dotenv(path: Path) -> None:
    """Minimal .env reader (KEY=VALUE, # comments, optional quotes).
    Variables already set in the environment win over the file."""
    if not path.is_file():
        return
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        key = key.strip()
        if key.startswith("export "):
            key = key[len("export "):].strip()
        value = value.strip()
        if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
            value = value[1:-1]
        os.environ.setdefault(key, value)


def build_message(device_id: str, app_secret: str, state: str, bad_signature: bool) -> bytes:
    payload = {
        "action": "setPowerState",
        "clientId": "lan-test",
        "createdAt": int(time.time()),  # the device adopts this as its clock, keep it real
        "deviceId": device_id,
        "replyToken": str(uuid.uuid4()),
        "scope": "device",
        "type": "request",
        "value": {"state": "On" if state == "on" else "Off"},
    }
    # The device signs/verifies the exact bytes between "payload": and ,"signature"
    payload_str = json.dumps(payload, separators=(",", ":"))
    key = app_secret + ("x" if bad_signature else "")
    digest = hmac.new(key.encode(), payload_str.encode(), hashlib.sha256).digest()
    signature = base64.b64encode(digest).decode()
    message = (
        '{"header":{"payloadVersion":2,"signatureVersion":1},'
        f'"payload":{payload_str},'
        f'"signature":{{"HMAC":"{signature}"}}}}'
    )
    return message.encode()


def main() -> int:
    load_dotenv(PROJECT_DIR / ".env")

    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("state", nargs="?", default="on", choices=["on", "off"])
    parser.add_argument("--ip", default=os.environ.get("SINRIC_DEVICE_IP"),
                        help="device IP (serial log: [WiFi]: Connected! IP: ...), default SINRIC_DEVICE_IP")
    parser.add_argument("--device-id", default=os.environ.get("SINRIC_DEVICE_ID"), help="default SINRIC_DEVICE_ID")
    parser.add_argument("--app-secret", default=os.environ.get("SINRIC_APP_SECRET"), help="default SINRIC_APP_SECRET")
    parser.add_argument("--bad-signature", action="store_true", help="sign with a wrong secret, the device must refuse")
    parser.add_argument("--timeout", type=float, default=3.0)
    args = parser.parse_args()

    missing = [name for name, value in (("SINRIC_DEVICE_IP", args.ip),
                                        ("SINRIC_DEVICE_ID", args.device_id),
                                        ("SINRIC_APP_SECRET", args.app_secret)) if not value]
    if missing:
        parser.error(f"missing {', '.join(missing)}: set them in {PROJECT_DIR / '.env'} (copy .env.example)")

    message = build_message(args.device_id, args.app_secret, args.state, args.bad_signature)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(args.timeout)
    sock.bind(("", 0))

    print(f"-> {args.ip}:{PORT}  setPowerState {args.state.upper()}"
          f"{'  (wrong signature)' if args.bad_signature else ''}")
    start = time.monotonic()
    sock.sendto(message, (args.ip, PORT))
    try:
        data, peer = sock.recvfrom(4096)
    except socket.timeout:
        print(f"!! no reply within {args.timeout}s: local control not running, wrong IP, "
              "wrong deviceId (unknown devices are ignored silently) or UDP blocked")
        return 1
    elapsed = (time.monotonic() - start) * 1000

    reply = json.loads(data)
    payload = reply.get("payload", {})
    print(f"<- {peer[0]}:{peer[1]}  {elapsed:.0f} ms")
    print(json.dumps(payload, indent=2, ensure_ascii=False))
    return 0 if payload.get("success") else 2


if __name__ == "__main__":
    sys.exit(main())
