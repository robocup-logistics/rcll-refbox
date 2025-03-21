#!/usr/bin/env python3

from scapy.all import sniff, get_if_list
from time import sleep
import threading

PORT = 4444
seen_ips = set()
lock = threading.Lock()

def start_sniffer(iface):
    print(f"Starting sniffer on {iface}")
    try:
        sniff(
            iface=iface,
            filter=f"udp and dst port {PORT}",
            prn=lambda pkt: handle_packet(pkt),
            store=0,
        )
    except Exception as e:
        print(f"Sniffer failed on {iface}: {e}")

def handle_packet(pkt):
    ip = pkt[0][1].src  # assuming Ether/IP/UDP
    with lock:
        if ip not in seen_ips:
            print(f"New IP detected: {ip}")
            seen_ips.add(ip)

if __name__ == "__main__":
    interfaces = get_if_list()
    print(f"Sniffing on interfaces: {interfaces}")

    threads = []
    for iface in interfaces:
        t = threading.Thread(target=start_sniffer, args=(iface,), daemon=True)
        t.start()
        threads.append(t)

    try:
        while True:
            sleep(0.2)
            pass  # keep main thread alive
    except KeyboardInterrupt:
        print("Stopping sniffers.")
