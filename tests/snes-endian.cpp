#include "gtest/gtest.h"
#include "../cores/snes/port.h"

#include <cstring>

TEST(SnesEndian, MatchesHostByteOrder) {
	const uint16 word = 1;
	const bool hostLittleEndian = *reinterpret_cast<const uint8*>(&word) == 1;
#ifdef LSB_FIRST
	EXPECT_TRUE(hostLittleEndian);
#else
	EXPECT_FALSE(hostLittleEndian);
#endif
}

TEST(SnesEndian, ReadsAndWritesLittleEndianWords) {
	alignas(uint32) uint8 bytes[] = {0x12, 0x34, 0x56, 0x78};
	EXPECT_EQ(READ_WORD(bytes), 0x3412);
	EXPECT_EQ(READ_3WORD(bytes), 0x563412);
	EXPECT_EQ(READ_DWORD(bytes), 0x78563412);
	WRITE_DWORD(bytes, 0x12345678);
	const uint8 expected[] = {0x78, 0x56, 0x34, 0x12};
	EXPECT_EQ(std::memcmp(bytes, expected, sizeof(bytes)), 0);
}
