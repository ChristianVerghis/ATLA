#!/usr/bin/env python3
"""Run a Python script inside the running Unreal Editor via remote execution.

Usage: python3 ue_run.py <script.py>
       python3 ue_run.py -c "unreal.log('hello')"
"""
import os
import sys
import time

# Engine install; override with UE_ROOT for a non-default location.
UE_ROOT = os.environ.get("UE_ROOT", "/Users/Shared/Epic Games/UE_5.8")
sys.path.insert(0, os.path.join(UE_ROOT, "Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python"))
import remote_execution as remote
import socket as _socket


def _patched_init_broadcast_socket(self):
    """macOS-safe variant: bind INADDR_ANY to receive multicast, send via loopback,
    and join the group on every viable interface."""
    s = _socket.socket(_socket.AF_INET, _socket.SOCK_DGRAM, _socket.IPPROTO_UDP)
    if hasattr(_socket, "SO_REUSEPORT"):
        s.setsockopt(_socket.SOL_SOCKET, _socket.SO_REUSEPORT, 1)
    s.setsockopt(_socket.SOL_SOCKET, _socket.SO_REUSEADDR, 1)
    s.bind(("0.0.0.0", self._config.multicast_group_endpoint[1]))
    s.setsockopt(_socket.IPPROTO_IP, _socket.IP_MULTICAST_LOOP, 1)
    s.setsockopt(_socket.IPPROTO_IP, _socket.IP_MULTICAST_TTL, self._config.multicast_ttl)
    s.setsockopt(_socket.IPPROTO_IP, _socket.IP_MULTICAST_IF, _socket.inet_aton("127.0.0.1"))
    group = _socket.inet_aton(self._config.multicast_group_endpoint[0])
    interfaces = ["127.0.0.1"]
    try:
        interfaces.append(_socket.gethostbyname(_socket.gethostname()))
    except OSError:
        pass
    for iface in interfaces:
        try:
            s.setsockopt(_socket.IPPROTO_IP, _socket.IP_ADD_MEMBERSHIP, group + _socket.inet_aton(iface))
        except OSError:
            pass
    s.settimeout(0.1)
    self._broadcast_socket = s


remote._RemoteExecutionBroadcastConnection._init_broadcast_socket = _patched_init_broadcast_socket


def _patched_broadcast_message(self, message):
    """Send every discovery/control message both as unicast to the editor's
    loopback-bound socket and to the multicast group."""
    data = message.to_json_bytes()
    port = self._config.multicast_group_endpoint[1]
    for dest in (("127.0.0.1", port), self._config.multicast_group_endpoint):
        try:
            self._broadcast_socket.sendto(data, dest)
        except OSError:
            pass


remote._RemoteExecutionBroadcastConnection._broadcast_message = _patched_broadcast_message


def main():
    if len(sys.argv) < 2:
        print("usage: ue_run.py <script.py> | -c <code>", file=sys.stderr)
        return 2

    if sys.argv[1] == "-c":
        code = sys.argv[2]
    else:
        with open(sys.argv[1]) as f:
            code = f.read()

    config = remote.RemoteExecutionConfig()
    # macOS: multicast doesn't reach sockets bound to 127.0.0.1
    config.multicast_bind_address = "0.0.0.0"
    re = remote.RemoteExecution(config)
    re.start()
    try:
        deadline = time.time() + 5.0
        while not re.remote_nodes and time.time() < deadline:
            time.sleep(0.1)
        nodes = re.remote_nodes
        if not nodes:
            print("ERROR: no Unreal Editor found (is it running with remote execution enabled?)", file=sys.stderr)
            return 1
        re.open_command_connection(nodes[0]["node_id"])
        result = re.run_command(code, unattended=True, exec_mode=remote.MODE_EXEC_FILE, raise_on_failure=False)
        for entry in result.get("output", []):
            print(f"[{entry['type']}] {entry['output']}", end="")
        if not result.get("success", False):
            print(f"\nCOMMAND FAILED: {result.get('result')}", file=sys.stderr)
            return 1
        return 0
    finally:
        re.stop()


if __name__ == "__main__":
    sys.exit(main())
