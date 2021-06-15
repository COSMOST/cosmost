#ifndef __STUDY_FFS_H__
#define __STUDY_FFS_H__

    /*
     * Count the consecutive zero bits (trailing) on the right with multiply and lookup
     * 
     * The following is based on
     * http://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightMultLookup
     */
    static inline
    int ffs_lsb(uint32_t v)
    {
        static const int MultiplyDeBruijnBitPosition[32] =
        {
          0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
          31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
        };

        if (!v)
            return 32;

        if (!(v ^ -1))
            return -1;

        return MultiplyDeBruijnBitPosition[((uint32_t)((v & -v) * 0x077CB531U)) >> 27];
    }
#endif
