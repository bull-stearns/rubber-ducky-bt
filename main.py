import asyncio
from bleak import BleakScanner, BleakClient
from selector import selector
import os

DATA_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"
STATE_UUID = "f0000001-0451-4000-b000-000000000000"
TOTALSIZE_UUID = "f0000001-0451-4000-b000-000000000001"

MAX_CHUNK_SIZE = 256

async def wait_for_state(client, expected):
    while True:
        state = await client.read_gatt_char(STATE_UUID)
        if state.decode() == expected:
            return
        await asyncio.sleep(0.05)

async def send_file(file_path):
    if not os.path.isfile(file_path):
        print(f"File not found: {file_path}")
        return

    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()
    data = content.encode()
    total_len = len(data)

    print(f"File size: {total_len} bytes")

    print("Scanning for BLE devices...")
    devices = await BleakScanner.discover(timeout=2.0)
    if not len(devices):
        raise LookupError("No BLE Devices found.")

    target = devices[await selector([(f"{d.name} ({d.address})" if d.name else d.address) for d in devices])]

    async with BleakClient(target.address) as client:
        print("Connected to:", target.name)

        await wait_for_state(client, "READY")
        await client.write_gatt_char(TOTALSIZE_UUID, str(total_len).encode())
        print("Sent total size")

        offset = 0
        while offset < total_len:
            await wait_for_state(client, "READY")
            chunk = data[offset:offset + MAX_CHUNK_SIZE]
            await client.write_gatt_char(DATA_UUID, chunk)
            await client.write_gatt_char(STATE_UUID, b"PROCESSING")
            offset += len(chunk)

        print("File sent.")

if __name__ == "__main__":
    file_path = input("Enter path to text file: ")
    asyncio.run(send_file(file_path))
