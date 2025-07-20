import asyncio
from qemu.qmp import QMPClient


async def main():
    qmp = QMPClient("my_virtual_machine_name")
    await qmp.connect(("localhost", 4444))

    res = await qmp.execute("query-block")
    print(f"{res}")
    await qmp.disconnect()


asyncio.run(main())
