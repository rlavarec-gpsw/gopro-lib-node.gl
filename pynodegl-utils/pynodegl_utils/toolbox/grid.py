#
# Copyright 2020 GoPro Inc.
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#

import math
from typing import Any, Sequence

import pynodegl as ngl


class AutoGrid:
    def __init__(self, elems: Sequence[Any]):
        self.elems = elems
        self.nb = len(elems)
        self.nb_rows = int(round(math.sqrt(self.nb)))
        self.nb_cols = int(math.ceil(self.nb / float(self.nb_rows)))
        self.dim = max(self.nb_rows, self.nb_cols)
        self.scale = 1.0 / self.dim

    def _get_coords(self, pos: tuple[float, float]) -> tuple[float, float]:
        col, row = pos
        pos_x = self.scale * (col * 2.0 + 1.0) - 1.0
        pos_y = self.scale * (row * -2.0 - 1.0) + 1.0
        return pos_x, pos_y

    def transform_coords(self, coords: tuple[float, float], pos: tuple[float, float]) -> tuple[float, float]:
        adjust = self._get_coords(pos)
        scales = (self.scale, self.scale)
        return tuple(c * s + a for c, s, a in zip(coords, scales, adjust))

    def place_node(self, node: ngl.Node, pos: tuple[float, float]):
        pos_x, pos_y = self._get_coords(pos)
        # This is equivalent to Translate(Scale(node))
        mat = (
            # fmt: off
            self.scale, 0, 0, 0,
            0, self.scale, 0, 0,
            0, 0, 1, 0,
            pos_x, pos_y, 0, 1,
            # fmt: on
        )
        return ngl.Transform(node, matrix=mat, label="grid(col=%d,row=%d)" % pos)

    def __iter__(self):
        i = 0
        for row in range(self.nb_rows):
            for col in range(self.nb_cols):
                yield self.elems[i], i, col, row
                i += 1
                if i == self.nb:
                    return


def autogrid_simple(scenes: Sequence[ngl.Node]) -> ngl.Node:
    ag = AutoGrid(scenes)
    g = ngl.Group()
    for scene, _, col, row in ag:
        scene = ag.place_node(scene, (col, row))
        g.add_children(scene)
    return g


def autogrid_queue(scenes: Sequence[ngl.Node], duration: float, overlap_time: float) -> ngl.Node:
    ag = AutoGrid(scenes)
    g = ngl.Group()
    for scene, scene_id, col, row in ag:
        if duration is not None:
            assert overlap_time is not None
            scene = ngl.TimeRangeFilter(scene)
            start = scene_id * duration / ag.nb
            if start:
                scene.add_ranges(ngl.TimeRangeModeNoop(0))
            scene.add_ranges(
                ngl.TimeRangeModeCont(start), ngl.TimeRangeModeNoop(start + duration / ag.nb + overlap_time)
            )
        scene = ag.place_node(scene, (col, row))
        g.add_children(scene)
    return g


if __name__ == "__main__":
    N = 10
    for n in range(1, N * N + 1):
        ag = AutoGrid(range(n))
        buf = [["."] * ag.nb_cols for _ in range(ag.nb_rows)]
        for _, i, x, y in ag:
            buf[y][x] = "x"
        print("#%d (cols:%d rows:%d)" % (n, ag.nb_cols, ag.nb_rows))
        print("\n".join("".join(line) for line in buf))
        print()
