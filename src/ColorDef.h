// Define Arduino Menu Colors

#define Black RGB565(0,0,0)
#define Red	RGB565(255,0,0)
#define Green RGB565(0,255,0)
#define Blue RGB565(0,0,255)
#define Gray RGB565(128,128,128)
#define LighterRed RGB565(255,150,150)
#define LighterGreen RGB565(150,255,150)
#define LighterBlue RGB565(150,150,255)
#define DarkerRed RGB565(150,0,0)
#define DarkerGreen RGB565(0,150,0)
#define DarkerBlue RGB565(0,0,150)
#define Cyan RGB565(0,255,255)
#define Magenta RGB565(255,0,255)
#define Yellow RGB565(255,255,0)
#define White RGB565(255,255,255)

const colorDef<uint16_t> colors[6] MEMMODE={
  {{(uint16_t)Black,(uint16_t)Black}, {(uint16_t)Black, (uint16_t)Black,  (uint16_t)Black}},//bgColor
  {{(uint16_t)Gray, (uint16_t)Gray},  {(uint16_t)Gray, (uint16_t)White, (uint16_t)White}},//fgColor
  {{(uint16_t)White,(uint16_t)Black}, {(uint16_t)Gray,(uint16_t)White,(uint16_t)Blue}},//valColor
  {{(uint16_t)White,(uint16_t)Black}, {(uint16_t)Gray, (uint16_t)White,(uint16_t)Blue}},//unitColor
  {{(uint16_t)White,(uint16_t)Gray},  {(uint16_t)Black, (uint16_t)Black,  (uint16_t)White}},//cursorColor
  {{(uint16_t)White,(uint16_t)Yellow},{(uint16_t)Black,  (uint16_t)Red,   (uint16_t)Red}},//titleColor
};