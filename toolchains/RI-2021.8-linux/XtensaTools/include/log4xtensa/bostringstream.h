// Copyright (c) 2005 by Tensilica Inc.  ALL RIGHTS RESERVED.
// These coded instructions, statements, and computer programs are the
// copyrighted works and confidential proprietary information of Tensilica Inc.
// They may not be modified, copied, reproduced, distributed, or disclosed to
// third parties in any manner, medium, or form, in whole or in part, without
// the prior written consent of Tensilica Inc.

#ifndef _BOSTRINGSTREAM_H_
#define _BOSTRINGSTREAM_H_

#include <string>
#include <sstream>
#include <ostream>
#include <istream>



namespace log4xtensa {


class LOG4XTENSA_EXPORT Sizer {};


/**
 * This class is a wrapper around a std::ostringstream object.  It works similar to 
 * std::ostringstream except that data is added to the stream in binary format (not
 * as text).
 */
class LOG4XTENSA_EXPORT bostringstream {
public:

    typedef unsigned char           u8;
    typedef   signed char           i8;
    typedef unsigned short          u16;
    typedef   signed short          i16;
    typedef unsigned int            u32;
    typedef   signed int            i32;
#ifdef _MSC_EXTENSIONS
    typedef unsigned __int64        u64;
    typedef   signed __int64        i64;
#else
    typedef unsigned long long      u64;
    typedef   signed long long      i64;
#endif

    bostringstream() {
      m_get_size = false;
      m_size = 4;
    }

    /// Return the string of binary data
    std::string str() { return m_oss.str(); }

    /// Return the size in bytes of the binary data
    size_t size() const { return m_oss.str().size(); }

    /// Append a u8 value to the binary stream
    bostringstream& operator<<(u8  v) { if (m_get_size) { m_size = (int) v; m_get_size = false; return *this; } return write(v); }
    /// Append a u16 value to the binary stream
    bostringstream& operator<<(u16 v) { if (m_get_size) { m_size = (int) v; m_get_size = false; return *this; } return write(v); }
    /// Append a u32 value to the binary stream
    bostringstream& operator<<(u32 v) { if (m_get_size) { m_size = (int) v; m_get_size = false; return *this; } return write(v); }
    /// Append a u64 value to the binary stream
    bostringstream& operator<<(u64 v) { if (m_get_size) { m_size = (int) v; m_get_size = false; return *this; } return write(v); }

    /// Append an i8 value to the binary stream
    bostringstream& operator<<(i8  v) { if (m_get_size) { m_size = (int) v; m_get_size = false; return *this; } return write(v); }
    /// Append an i16 value to the binary stream
    bostringstream& operator<<(i16 v) { if (m_get_size) { m_size = (int) v; m_get_size = false; return *this; } return write(v); }
    /// Append an i32 value to the binary stream
    bostringstream& operator<<(i32 v) { if (m_get_size) { m_size = (int) v; m_get_size = false; return *this; } return write(v); }
    /// Append an i64 value to the binary stream
    bostringstream& operator<<(i64 v) { if (m_get_size) { m_size = (int) v; m_get_size = false; return *this; } return write(v); }

    /// Append m_size bytes from pv to the binary stream (@see m_size)
    bostringstream& operator<<(const void *pv) { m_oss.write(reinterpret_cast<const char *>(pv), m_size); return *this; }

    /// Append m_size bytes from pv to the binary stream (@see m_size)
    bostringstream& operator<<(const u8   *pv) { m_oss.write(reinterpret_cast<const char *>(pv), m_size); return *this; }
    /// Append m_size bytes from pv to the binary stream (@see m_size)
    bostringstream& operator<<(const u16  *pv) { m_oss.write(reinterpret_cast<const char *>(pv), m_size); return *this; }
    /// Append m_size bytes from pv to the binary stream (@see m_size)
    bostringstream& operator<<(const u32  *pv) { m_oss.write(reinterpret_cast<const char *>(pv), m_size); return *this; }
    /// Append m_size bytes from pv to the binary stream (@see m_size)
    bostringstream& operator<<(const u64  *pv) { m_oss.write(reinterpret_cast<const char *>(pv), m_size); return *this; }

    /// Append m_size bytes from pv to the binary stream (@see m_size)
    bostringstream& operator<<(const i8   *pv) { m_oss.write(reinterpret_cast<const char *>(pv), m_size); return *this; }
    /// Append m_size bytes from pv to the binary stream (@see m_size)
    bostringstream& operator<<(const i16  *pv) { m_oss.write(reinterpret_cast<const char *>(pv), m_size); return *this; }
    /// Append m_size bytes from pv to the binary stream (@see m_size)
    bostringstream& operator<<(const i32  *pv) { m_oss.write(reinterpret_cast<const char *>(pv), m_size); return *this; }
    /// Append m_size bytes from pv to the binary stream (@see m_size)
    bostringstream& operator<<(const i64  *pv) { m_oss.write(reinterpret_cast<const char *>(pv), m_size); return *this; }

    /// Indicates that the next integer is to be stored as m_size (and not itself added to the output binary stream)
    bostringstream& operator<<(const Sizer& s) { (void) s; m_get_size = true; return *this; }

  protected:

    bostringstream& write(u8  v) { m_oss.write(reinterpret_cast<char *>(&v), 1); return *this; }
    bostringstream& write(i8  v) { m_oss.write(reinterpret_cast<char *>(&v), 1); return *this; }

#if defined(__linux__) || defined(_WIN32)
    // Little endian host (no swizzle required)
    bostringstream& write(u16 v) { m_oss.write(reinterpret_cast<char *>(&v), 2); return *this; }
    bostringstream& write(u32 v) { m_oss.write(reinterpret_cast<char *>(&v), 4); return *this; }
    bostringstream& write(u64 v) { m_oss.write(reinterpret_cast<char *>(&v), 8); return *this; }

    bostringstream& write(i16 v) { m_oss.write(reinterpret_cast<char *>(&v), 2); return *this; }
    bostringstream& write(i32 v) { m_oss.write(reinterpret_cast<char *>(&v), 4); return *this; }
    bostringstream& write(i64 v) { m_oss.write(reinterpret_cast<char *>(&v), 8); return *this; }

#else
    // Big endian host: __sparc__  (need to swizzle)
    bostringstream& write(u16 v) {
      u16 s = (((v & 0x00FF) << 8) |
               ((v & 0xFF00) >> 8));
      m_oss.write(reinterpret_cast<char *>(&s), 2);
      return *this;
    }
    bostringstream& write(u32 v) {
      u32 s = (((v & 0x000000FF) << 24) |
               ((v & 0x0000FF00) << 8 ) |
               ((v & 0x00FF0000) >> 8 ) |
               ((v & 0xFF000000) >> 24));
      m_oss.write(reinterpret_cast<char *>(&s), 4);
      return *this;
    }
    bostringstream& write(u64 v) {
      u32 high = (u32) ((v & 0xFFFFFFFF00000000LL) >> 32);
      u32 low  = (u32)  (v & 0x00000000FFFFFFFFLL);
      write(high);
      write(low);
      return *this;
    }

    bostringstream& write(i16 v) { return write((u16) v); }
    bostringstream& write(i32 v) { return write((u32) v); }
    bostringstream& write(i64 v) { return write((u64) v); }
#endif

    std::ostringstream m_oss;   // The actual stream object

    /**
     * The size of data pointed to by pointers (default value is 4).  m_size can be changed
     * midstream to N bytes by adding "<< sizer << N" to the stream.
     */
    int  m_size;

    bool m_get_size; // Indicates that next integer is a size specifier for pointer data
};


} // namespace log4xtensa

#endif  // _BOSTRINGSTREAM_H_
