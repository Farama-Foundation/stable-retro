#!/usr/bin/env python
import argparse
import csv
import os
import shlex
import signal
import socket
import subprocess
import sys
import time


class PerfTest:
    EXECUTABLE = "mgba-perf"

    def __init__(self, rom, renderer="software"):
        self.rom = rom
        self.renderer = renderer
        self.results = None
        self.name = f"Perf Test: {rom}"

    def get_args(self):
        return []

    def wait(self, proc):
        pass

    def run(self, cwd):
        args = [os.path.join(os.getcwd(), self.EXECUTABLE), "-P"]
        args.extend(self.get_args())
        if self.renderer != "software":
            args.append("-N")
        args.append(self.rom)
        env = {}
        if "LD_LIBRARY_PATH" in os.environ:
            env["LD_LIBRARY_PATH"] = os.path.abspath(os.environ["LD_LIBRARY_PATH"])
            env["DYLD_LIBRARY_PATH"] = env["LD_LIBRARY_PATH"]  # Fake it on OS X
        proc = subprocess.Popen(
            args, stdout=subprocess.PIPE, cwd=cwd, universal_newlines=True, env=env
        )
        try:
            self.wait(proc)
            proc.wait()
        except Exception:
            proc.kill()
            raise
        if proc.returncode:
            print("Game crashed!", file=sys.stderr)
            return
        reader = csv.DictReader(proc.stdout)
        self.results = next(reader)


class WallClockTest(PerfTest):
    def __init__(self, rom, duration, renderer="software"):
        super().__init__(rom, renderer)
        self.duration = duration
        self.name = f"Wall-Clock Test ({duration} seconds, {renderer} renderer): {rom}"

    def wait(self, proc):
        time.sleep(self.duration)
        proc.send_signal(signal.SIGINT)


class GameClockTest(PerfTest):
    def __init__(self, rom, frames, renderer="software"):
        super().__init__(rom, renderer)
        self.frames = frames
        self.name = f"Game-Clock Test ({frames} frames, {renderer} renderer): {rom}"

    def get_args(self):
        return ["-F", str(self.frames)]


class PerfServer:
    ITERATIONS_PER_INSTANCE = 50

    def __init__(self, address, command=None):
        s = address.rsplit(":", 1)
        if len(s) == 1:
            self.address = (s[0], 7216)
        else:
            self.address = (s[0], s[1])
        if command:
            self.command = shlex.split(command)
        self.iterations = self.ITERATIONS_PER_INSTANCE
        self.socket = None
        self.results = []
        self.reader = None

    def _start(self, test):
        if self.command:
            server_command = list(self.command)
        else:
            server_command = [os.path.join(os.getcwd(), PerfTest.EXECUTABLE)]
        server_command.extend(["--", "-PD", "0"])
        if hasattr(test, "frames"):
            server_command.extend(["-F", str(test.frames)])
        if test.renderer != "software":
            server_command.append("-N")
        subprocess.check_call(server_command)
        time.sleep(4)
        self.socket = socket.create_connection(self.address, timeout=1000)
        self.reader = csv.DictReader(self.socket.makefile())

    def run(self, test):
        if not self.socket:
            self._start(test)
        self.socket.send(os.path.join("/perfroms", test.rom))
        self.results.append(next(self.reader))
        self.iterations -= 1
        if self.iterations == 0:
            self.finish()
            self.iterations = self.ITERATIONS_PER_INSTANCE

    def finish(self):
        self.socket.send("\n")
        self.reader = None
        self.socket.close()
        time.sleep(5)
        self.socket = None


class Suite:
    def __init__(self, cwd, wall=None, game=None, renderer="software"):
        self.cwd = cwd
        self.tests = []
        self.wall = wall
        self.game = game
        self.renderer = renderer
        self.server = None

    def set_server(self, server):
        self.server = server

    def collect_tests(self):
        roms = []
        for f in os.listdir(self.cwd):
            if (
                f.endswith(".gba")
                or f.endswith(".zip")
                or f.endswith(".gbc")
                or f.endswith(".gb")
            ):
                roms.append(f)
        roms.sort()
        for rom in roms:
            self.add_tests(rom)

    def add_tests(self, rom):
        if self.wall:
            self.tests.append(WallClockTest(rom, self.wall, renderer=self.renderer))
        if self.game:
            self.tests.append(GameClockTest(rom, self.game, renderer=self.renderer))

    def run(self):
        results = []
        for test in self.tests:
            print(f"Running test {test.name}", file=sys.stderr)
            if self.server:
                self.server.run(test)
            else:
                try:
                    test.run(self.cwd)
                except KeyboardInterrupt:
                    print("Interrupted, returning early...", file=sys.stderr)
                    return results
                if test.results:
                    results.append(test.results)
        if self.server:
            self.server.finish()
            results.extend(self.server.results)
        return results


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-w",
        "--wall-time",
        type=float,
        default=0,
        metavar="TIME",
        help="wall-clock time",
    )
    parser.add_argument(
        "-g",
        "--game-frames",
        type=int,
        default=0,
        metavar="FRAMES",
        help="game-clock frames",
    )
    parser.add_argument(
        "-N",
        "--disable-renderer",
        action="store_const",
        const=True,
        help="disable video rendering",
    )
    parser.add_argument("-s", "--server", metavar="ADDRESS", help="run on server")
    parser.add_argument(
        "-S", "--server-command", metavar="COMMAND", help="command to launch server"
    )
    parser.add_argument("-o", "--out", metavar="FILE", help="output file path")
    parser.add_argument("directory", help="directory containing ROM files")
    args = parser.parse_args()

    s = Suite(
        args.directory,
        wall=args.wall_time,
        game=args.game_frames,
        renderer=None if args.disable_renderer else "software",
    )
    if args.server:
        if args.server_command:
            server = PerfServer(args.server, args.server_command)
        else:
            server = PerfServer(args.server)
        s.set_server(server)
    s.collect_tests()
    results = s.run()
    fout = sys.stdout
    if args.out:
        fout = open(args.out, "w")
    writer = csv.DictWriter(fout, results[0].keys())
    writer.writeheader()
    writer.writerows(results)
    if fout is not sys.stdout:
        fout.close()
