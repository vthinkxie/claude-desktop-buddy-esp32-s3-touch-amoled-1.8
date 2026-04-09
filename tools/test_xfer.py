#!/usr/bin/env python3
"""
Prove the Stick's serial file receiver works: stream an existing character
back over serial, watch the acks, verify it reloads.
"""
import sys, json, base64, time, glob, os, serial

CHUNK = 256
PORT = (glob.glob('/dev/cu.usbserial-*') + [None])[0]
if not PORT: sys.exit("no stick found")

s = serial.Serial(PORT, 115200, timeout=2)
s.dtr = True; s.rts = False
time.sleep(2)   # let any DTR-triggered reset finish booting
s.reset_input_buffer()

def send(obj):
    s.write((json.dumps(obj) + "\n").encode())

def wait_ack(what, timeout=5):
    deadline = time.time() + timeout
    while time.time() < deadline:
        line = s.readline().decode('utf-8', errors='replace').strip()
        if not line: continue
        if line.startswith('{'):
            try:
                a = json.loads(line)
                if a.get('ack') == what:
                    return a
            except: pass
        else:
            print(f"  (skip: {line[:60]})", file=sys.stderr)
    return None

def send_file(name, path):
    data = open(path, 'rb').read()
    print(f"  {name}: {len(data)} bytes", end='', flush=True)
    send({"cmd":"file", "path":name, "size":len(data)})
    a = wait_ack("file")
    if not a or not a.get('ok'): print(" — open FAILED"); return False

    for i in range(0, len(data), CHUNK):
        chunk = data[i:i+CHUNK]
        send({"cmd":"chunk", "d": base64.b64encode(chunk).decode()})
        a = wait_ack("chunk", timeout=3)
        if not a or not a.get('ok'):
            print(f" — chunk {i} FAILED"); return False
        if i and i % 16384 == 0: print(".", end="", flush=True)

    send({"cmd":"file_end"})
    a = wait_ack("file_end", timeout=10)
    ok = a and a.get('ok') and a.get('n') == len(data)
    print(f" — {'ok' if ok else 'FAILED'} ({a.get('n') if a else '?'} written)")
    return ok

src = sys.argv[1] if len(sys.argv) > 1 else f"{os.path.dirname(__file__)}/../characters/bufo"
name = sys.argv[2] if len(sys.argv) > 2 else "test"

print(f"installing '{name}' from {src}")
print("waiting for Stick...", end='', flush=True)
for attempt in range(8):
    s.reset_input_buffer()
    send({"cmd":"char_begin", "name":name})
    a = wait_ack("char_begin", timeout=2)
    if a and a.get('ok'):
        print(" ready")
        break
    print(".", end='', flush=True)
    time.sleep(1)
else:
    sys.exit("\nchar_begin: Stick never responded")

t0 = time.time()
files = sorted(glob.glob(f"{src}/*"))
total = sum(os.path.getsize(f) for f in files)
print(f"{len(files)} files, {total} bytes total")

for f in files:
    if not send_file(os.path.basename(f), f):
        sys.exit("transfer failed")

send({"cmd":"char_end"})
a = wait_ack("char_end", timeout=10)
dt = time.time() - t0
print(f"\nchar_end: {a}")
print(f"{total} bytes in {dt:.1f}s = {total/dt/1024:.1f} KB/s")
s.close()
