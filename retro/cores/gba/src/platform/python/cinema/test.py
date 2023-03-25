import glob
import itertools
import os
import os.path
import re
from copy import deepcopy

import cinema.movie
import mgba.core
import mgba.image
import yaml
from cinema import VideoFrame
from cinema.util import dictMerge


class CinemaTest:
    TEST = "test.(mvl|gb|gba|nds)"

    def __init__(self, path, root, settings={}):
        self.fullPath = path or []
        self.path = os.path.abspath(os.path.join(root, *self.fullPath))
        self.root = root
        self.name = ".".join(path)
        self.settings = settings
        try:
            with open(os.path.join(self.path, "manifest.yml")) as f:
                dictMerge(self.settings, yaml.safe_load(f))
        except OSError:
            pass
        self.tests = {}

    def __repr__(self):
        return f"<{self.__class__.__name__} {self.name}>"

    def setUp(self):
        results = [
            f
            for f in glob.glob(os.path.join(self.path, "test.*"))
            if re.search(self.TEST, f)
        ]
        self.core = mgba.core.loadPath(results[0])
        if "config" in self.settings:
            self.config = mgba.core.Config(defaults=self.settings["config"])
            self.core.loadConfig(self.config)
            self.core.reset()

    def addTest(self, name, cls=None, settings={}):
        cls = cls or self.__class__
        newSettings = deepcopy(self.settings)
        dictMerge(newSettings, settings)
        self.tests[name] = cls(self.fullPath + [name], self.root, newSettings)
        return self.tests[name]

    def outputSettings(self):
        outputSettings = {}
        if "frames" in self.settings:
            outputSettings["limit"] = self.settings["frames"]
        if "skip" in self.settings:
            outputSettings["skip"] = self.settings["skip"]
        return outputSettings

    def __lt__(self, other):
        return self.path < other.path


class VideoTest(CinemaTest):
    BASELINE = "baseline_%04u.png"

    def setUp(self):
        super().setUp()
        self.tracer = cinema.movie.Tracer(self.core)

    def generateFrames(self):
        for i, frame in zip(
            itertools.count(), self.tracer.video(**self.outputSettings())
        ):
            try:
                baseline = VideoFrame.load(os.path.join(self.path, self.BASELINE % i))
                yield baseline, frame, VideoFrame.diff(baseline, frame)
            except OSError:
                yield None, frame, (None, None)

    def test(self):
        self.baseline, self.frames, self.diffs = zip(*self.generateFrames())
        assert not any(
            any(diffs[0].image.convert("L").point(bool).getdata())
            for diffs in self.diffs
        )

    def generateBaseline(self):
        for i, frame in zip(
            itertools.count(), self.tracer.video(**self.outputSettings())
        ):
            frame.save(os.path.join(self.path, self.BASELINE % i))


def gatherTests(root=os.getcwd()):
    tests = CinemaTest([], root)
    for path, _, files in os.walk(root):
        test = [f for f in files if re.match(CinemaTest.TEST, f)]
        if not test:
            continue
        prefix = os.path.commonprefix([path, root])
        suffix = path[len(prefix) + 1 :]
        testPath = suffix.split(os.sep)
        testRoot = tests
        for component in testPath[:-1]:
            newTest = testRoot.tests.get(component)
            if not newTest:
                newTest = testRoot.addTest(component)
            testRoot = newTest
        testRoot.addTest(testPath[-1], VideoTest)
    return tests
