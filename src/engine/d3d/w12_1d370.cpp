// *********************************************************************** //
//
//  w12_1d370.cpp -- 0x1001d370 (28 bytes): CMoArray<T,C>::operator[] of
//  d3dr4b.cpp's 32-byte-element array (Arr32), `return Get(index);` with
//  Get (0x10009ef0) out of line -- the shipped body is `mov 8(%ebp),%eax;
//  mov %eax,(%esp); mov %ecx,%eax; mov %eax,%ecx; call 0x10009ef0; ret $4`.
//  d3dr4b.cpp's GenCopyList/GenAppendList call it as `sub_1001d370(i) = x`.
//  Get is aliased onto d3dbatchc_o2.cpp's C10009ef0::sub_10009ef0 (the
//  same address, identical) in link/aliases.rsp.
//
// *********************************************************************** //

typedef unsigned long uint32;

struct T32 { uint32 m_v[8]; };

struct Arr32
{
	T32&	sub_10009ef0(uint32 nIndex) const;          // 0x10009ef0 Get (declared only)
	T32&	sub_1001d370(uint32 nIndex) const;          // 0x1001d370 operator[]
};

T32& Arr32::sub_1001d370(uint32 nIndex) const
{
	return sub_10009ef0(nIndex);
}
