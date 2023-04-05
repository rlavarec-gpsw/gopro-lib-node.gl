/*
 * Copyright 2016 GoPro Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stdafx.h>
#include "topology_d3d12.h"
#include "topology.h"


static const std::map<uint32_t, D3D_PRIMITIVE_TOPOLOGY> ngfx_primitive_topology_map = {
    {NGLI_PRIMITIVE_TOPOLOGY_POINT_LIST,     D3D_PRIMITIVE_TOPOLOGY_POINTLIST    },
    {NGLI_PRIMITIVE_TOPOLOGY_LINE_LIST,      D3D_PRIMITIVE_TOPOLOGY_LINELIST     },
    {NGLI_PRIMITIVE_TOPOLOGY_LINE_STRIP,     D3D_PRIMITIVE_TOPOLOGY_LINESTRIP    },
    {NGLI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,  D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST },
    {NGLI_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP},
};

D3D_PRIMITIVE_TOPOLOGY to_d3d12_topology(int topology)
{
    return ngfx_primitive_topology_map.at(topology);
}
