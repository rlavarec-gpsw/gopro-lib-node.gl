/*
 * Copyright 2021-2022 GoPro Inc.
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
#include <stdlib.h>

#include "config.h"

extern "C" {

    extern const struct hwmap_class ngli_hwmap_vaapi_d3d12_class;

    const struct hwmap_class* ngli_hwmap_d3d12_classes[] = {
    #if defined(HAVE_VAAPI)
        &ngli_hwmap_vaapi_d3d12_class,
    #endif
        NULL
    };

}