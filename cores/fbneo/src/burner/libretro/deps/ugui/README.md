This is another form based on https://github.com/deividAlfa/UGUI with modifications:
1. Extended UTF8 support, no longer limited by the original algorithm's 0X8000 high bit flag method, which caused confusion for CJK characters whose unicode fall over 0X8000.
2. now the single character range for font conversion should be repeated once so that font range will appear in pairs.(unicode 169 has an offset:0x00,0xA9,0x00,0xA9 Flag:0x01,0x00)
3. Limited modification: The ".is_old_font" code still works,but font array need to update to new structure.
4. New C structure font array.
5. Ensure that the str pointer is correctly updated in the _UG_DecodeUTF8 function. This typically involves appropriately incrementing the pointer after identifying the number of bytes in the character.
6. Chinese Font is converted from Source Han Sans CN(Tool:https://github.com/agugu2000/ttf2ugui)
7. Simulation works normal for CJK characters
<img src="./ugui.png" width="600">


------------------------------------------------------------------------------------------------
## License and Disclaimer for Source Han Sans CN

This project uses the Source Han Sans CN font, which is licensed under the SIL Open Font License, Version 1.1.

### SIL Open Font License, Version 1.1

This Font Software is licensed under the SIL Open Font License, Version 1.1. This license is copied below, and is also available with a FAQ at: http://scripts.sil.org/OFL

#### Preamble

The goals of the Open Font License (OFL) are to stimulate worldwide development of collaborative font projects, to support the font creation efforts of academic and linguistic communities, and to provide a free and open framework in which fonts may be shared and improved in partnership with others.

The OFL allows the licensed fonts to be used, studied, modified, and redistributed freely as long as they are not sold by themselves. The fonts, including any derivative works, can be bundled, embedded, redistributed, and/or sold with any software provided that any reserved names are not used by derivative works. The fonts and derivatives, however, cannot be released under any other type of license. The requirement for fonts to remain under this license does not apply to any document created using the fonts or their derivatives.

#### Definitions

"Font Software" refers to the set of files released by the Copyright Holder(s) under this license and clearly marked as such. This may include source files, build scripts, and documentation.

"Reserved Font Name" refers to any names specified as such after the copyright statement(s).

"Original Version" refers to the collection of Font Software components as distributed by the Copyright Holder(s).

"Modified Version" refers to any derivative made by adding to, deleting, or substituting -- in part or in whole -- any of the components of the Original Version, by changing formats or by porting the Font Software to a new environment.

"Author" refers to any designer, engineer, programmer, technical writer, or other person who contributed to the Font Software.

#### Permission & Conditions

Permission is hereby granted, free of charge, to any person obtaining a copy of the Font Software, to use, study, copy, merge, embed, modify, redistribute, and sell modified and unmodified copies of the Font Software, subject to the following conditions:

1. Neither the Font Software nor any of its individual components, in Original or Modified Versions, may be sold by itself.
2. Original or Modified Versions of the Font Software may be bundled, redistributed, and/or sold with any software, provided that each copy contains the above copyright notice and this license. These can be included either as stand-alone text files, human-readable headers, or in the appropriate machine-readable metadata fields within text or binary files as long as those fields can be easily viewed by the user.
3. No Modified Version of the Font Software may use the Reserved Font Name(s) unless explicit written permission is granted by the corresponding Copyright Holder. This restriction applies to the Reserved Font Name(s) as well as any names that are confusingly similar to the Reserved Font Name(s).
4. The name(s) of the Copyright Holder(s) or the Author(s) of the Font Software shall not be used to promote, endorse, or advertise any Modified Version, except to acknowledge the contribution(s) of the Copyright Holder(s) and the Author(s) or with their explicit written permission.
5. The Font Software, modified or unmodified, in part or in whole, must be distributed entirely under this license, and must not be distributed under any other license. The requirement for fonts to remain under this license does not apply to any document created using the Font Software.

#### Termination

This license becomes null and void if any of the above conditions are not met.

#### DISCLAIMER

THE FONT SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT OF COPYRIGHT, PATENT, TRADEMARK, OR OTHER RIGHT. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES, OR OTHER LIABILITY, INCLUDING ANY GENERAL, SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, WHETHER IN AN ACTION OF CONTRACT, TORT, OR OTHERWISE, ARISING FROM, OUT OF THE USE OR INABILITY TO USE THE FONT SOFTWARE OR FROM OTHER DEALINGS IN THE FONT SOFTWARE.

### Usage in This Project

This project includes the Source Han Sans CN font. The font files are included in this repository in compliance with the SIL Open Font License, Version 1.1. You can find the font files in the `fonts` directory of this repository.

By using of this repository.

By using this font in this project, we comply with the SIL Open Font License, Version 1.1. For more information about the license, please refer to the [OFL FAQ](http://scripts.sil.org/OFL-FAQ).

### Disclaimer

The maintainers of this project are not responsible for any issues arising from the use of the Source Han Sans CN font. The font is provided "as is" without any warranty of any kind. For any legal concerns, please refer to the full text of the SIL Open Font License, Version 1.1.

------------------------------------------------------------------------------------------------

This is a forked version adding several enhancements:<br>
- Code reworked using [0x3333](https://github.com/0x3333/UGUI) UGUI fork.
- New font structure and functions.<br>
Fonts no longer require sequential characters, now they can have single chars and ranges, also support UTF8.<br>
This allows font stripping, saving a lot of space.<br>
- Add triangle drawing
- Add bmp acceleration (So the bmp data can be send using DMA), or use FILL_AREA driver if available.<br>
- Add 1BPP bmp drawing.
- 1BPP fonts can be drawn in transparent mode.<br>
- Modify FILL_AREA diver to allow passing multiple pixels at once.
- Font pixels are packed and only drawed when a different color is found.<br>
  This greatly enhances speed, removing a lot of overhead, specially when drawing big fonts.<br>



# Introduction
## What is µGUI?
µGUI is a free and open source graphic library for embedded systems. It is platform-independent
and can be easily ported to almost any microcontroller system. As long as the display is capable
of showing graphics, µGUI is not restricted to a certain display technology. Therefore, display
technologies such as LCD, TFT, E-Paper, LED or OLED are supported.

## µGUI Features
* µGUI supports any color, grayscale or monochrome display
* µGUI supports any display resolution
* µGUI supports multiple different displays
* µGUI supports any touch screen technology (e.g. AR, PCAP)
* µGUI supports windows and objects (e.g. button, textbox)
* µGUI supports platform-specific hardware acceleration
* Custom fonts can be added easily, several included by default, including cyrillic.
* TrueType font converter available: [ttf2uGUI](https://github.com/deividalfa/ttf2ugui)
* integrated and free scalable system console
* basic geometric functions (e.g. line, circle, frame etc.)
* can be easily ported to almost any microcontroller system
* no risky dynamic memory allocation required

## µGUI Requirements
µGUI is platform-independent, so there is no need to use a certain embedded system. In order to
use µGUI, only two requirements are necessary:
* a C-function which is able to control pixels of the target display.
* integer types for the target platform have to be adjusted in ugui_config.h.
