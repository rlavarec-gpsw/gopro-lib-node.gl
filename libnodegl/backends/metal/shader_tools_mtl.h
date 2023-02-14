/*
 * Copyright 2023 GoPro Inc.
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

#ifndef SHADER_TOOLS_MTL_H
#define SHADER_TOOLS_MTL_H

#include <cstddef>
#include <mutex>
#include <string>

class ShaderToolsMSL {
public:
    static void initialize();
    static void finalize();
    static bool compileGLSLToSpirv(int stage, const std::string &glsl_data,
                                   std::string &spv_data);
    static bool convertSpirvToMSL(const std::string &spv_data, std::string &msl_data);
    static void writeToFile(const std::string &filename, std::string &data);
    static void compileMSL(const std::string &filename);

private:
    static std::mutex mutex;
    static bool glsl_initialized;
};

#endif
