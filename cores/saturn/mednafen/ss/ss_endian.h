#include <algorithm>

#include "../mednafen-endian.h"

//
// Native endian.
//
template<typename T, bool aligned = false>
static INLINE T MDFN_densb(const void* ptr)
{
 return MDFN_deXsb<-1, T, aligned>(ptr);
}

template<typename T>
static INLINE uint8* ne64_ptr_be(uint64* const base, const size_t byte_offset)
{
#ifdef MSB_FIRST
  return (uint8*)base + (byte_offset &~ (sizeof(T) - 1));
#else
  return (uint8*)base + (((byte_offset &~ (sizeof(T) - 1)) ^ (8 - sizeof(T))));
#endif
}

template<typename T>
static INLINE void ne64_wbo_be(uint64* const base, const size_t byte_offset, const T value)
{
  uint8* const ptr = ne64_ptr_be<T>(base, byte_offset);

  static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8, "Unsupported type size");

  memcpy(MDFN_ASSUME_ALIGNED(ptr, sizeof(T)), &value, sizeof(T));
}

template<typename T, typename BT>
static INLINE uint8* ne16_ptr_be(BT* const base, const size_t byte_offset)
{
#ifdef MSB_FIRST
  return (uint8*)base + (byte_offset &~ (sizeof(T) - 1));
#else
  return (uint8*)base + (((byte_offset &~ (sizeof(T) - 1)) ^ (2 - std::min<size_t>(2, sizeof(T)))));
#endif
}

template<typename T>
static INLINE T ne16_rbo_be(const uint16* const base, const size_t byte_offset)
{
  uint8* const ptr = ne16_ptr_be<T>(base, byte_offset);

  static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4, "Unsupported type size");

  if(sizeof(T) == 4)
  {
   uint16* const ptr16 = (uint16*)ptr;
   T tmp;

   tmp  = ptr16[0] << 16;
   tmp |= ptr16[1];

   return tmp;
  }
  else
   return *(T*)ptr;
}

template<typename T>
static INLINE void ne16_wbo_be(uint16* const base, const size_t byte_offset, const T value)
{
  uint8* const ptr = ne16_ptr_be<T>(base, byte_offset);

  static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4, "Unsupported type size");

  if(sizeof(T) == 4)
  {
   uint16* const ptr16 = (uint16*)ptr;

   ptr16[0] = value >> 16;
   ptr16[1] = value;
  }
  else
   *(T*)ptr = value;
}

template<typename T>
static INLINE T ne64_rbo_be(uint64* const base, const size_t byte_offset)
{
  uint8* const ptr = ne64_ptr_be<T>(base, byte_offset);
  T ret;

  static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4, "Unsupported type size");

  memcpy(&ret, MDFN_ASSUME_ALIGNED(ptr, sizeof(T)), sizeof(T));

  return ret;
}

template<typename T, bool IsWrite>
static INLINE void ne64_rwbo_be(uint64* const base, const size_t byte_offset, T* value)
{
  if(IsWrite)
   ne64_wbo_be<T>(base, byte_offset, *value);
  else
   *value = ne64_rbo_be<T>(base, byte_offset);
}

template<typename T, bool IsWrite>
static INLINE void ne16_rwbo_be(uint16* const base, const size_t byte_offset, T* value)
{
  if(IsWrite)
   ne16_wbo_be<T>(base, byte_offset, *value);
  else
   *value = ne16_rbo_be<T>(base, byte_offset);
}

//
// X endian.
//
template<int isbigendian, typename T, bool aligned>
static INLINE void MDFN_enXsb(void* ptr, T value)
{
 T tmp = value;

 if(isbigendian != -1 && isbigendian != MDFN_ENDIANH_IS_BIGENDIAN)
 {
  static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8, "Gummy penguins.");

  if(sizeof(T) == 8)
   tmp = MDFN_bswap64(value);
  else if(sizeof(T) == 4)
   tmp = MDFN_bswap32(value);
  else if(sizeof(T) == 2)
   tmp = MDFN_bswap16(value);
 }

 memcpy(MDFN_ASSUME_ALIGNED(ptr, (aligned ? sizeof(T) : 1)), &tmp, sizeof(T));
}

//
// Native endian.
//
template<typename T, bool aligned = false>
static INLINE void MDFN_ennsb(void* ptr, T value)
{
 MDFN_enXsb<-1, T, aligned>(ptr, value);
}

template<typename T, bool aligned = false>
static INLINE void MDFN_enmsb(void* ptr, T value)
{
 MDFN_enXsb<1, T, aligned>(ptr, value);
}

template<bool aligned = false>
static INLINE void MDFN_en64msb(void* ptr, uint64 value)
{
 MDFN_enmsb<uint64, aligned>(ptr, value);
}
