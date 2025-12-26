/*
    Copyright 2016-2019 Arisotura

    This file is part of melonDS.

    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

const char* vertex_shader = R"(#version 140
layout(std140) uniform uConfig
{
    vec2 uScreenSize;
    uint u3DScale;
    uint uFilterMode;
    vec4 cursorPos;
};
in vec2 pos;
in vec2 texcoord;
smooth out vec2 fTexcoord;
void main()
{
    vec4 fpos;
    fpos.xy = ((pos * 2.0) / uScreenSize) - 1.0;
    fpos.y *= -1;
    fpos.z = 0.0;
    fpos.w = 1.0;
    gl_Position = fpos;
    fTexcoord = texcoord;
}
)";

const char* fragment_shader = R"(#version 140
layout(std140) uniform uConfig
{
    vec2 uScreenSize;
    uint u3DScale;
    uint uFilterMode;
    vec4 cursorPos;
};

uniform sampler2D ScreenTex;

smooth in vec2 fTexcoord;

out vec4 oColor;

void main()
{
    vec4 pixel = texture(ScreenTex, fTexcoord);

    // virtual cursor so you can see where you touch
    if(fTexcoord.y >= 0.5 && fTexcoord.y <= 1.0) {
        if(cursorPos.x <= fTexcoord.x && cursorPos.y <= fTexcoord.y && cursorPos.z >= fTexcoord.x && cursorPos.w >= fTexcoord.y) {
            pixel = vec4(1.0 - pixel.r, 1.0 - pixel.g, 1.0 - pixel.b, pixel.a);
        }
    }

    oColor = vec4(pixel.bgr, 1.0);
}
)";