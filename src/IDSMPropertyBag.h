#ifndef _IDSMPROPERTYBAG_H_
#define _IDSMPROPERTYBAG_H_

DEFINE_GUID(IID_IDSMChapterBag, 0x2D0EBE73, 0xBA82, 0x4E90, 0x85, 0x9B, 0xC7, 0xC4, 0x8E, 0xDE, 0x65, 0x0F);

DECLARE_INTERFACE_(IDSMChapterBag, IUnknown)
{
    STDMETHOD_(DWORD, ChapGetCount)() = 0;
    STDMETHOD(ChapGet)(DWORD iIndex, REFERENCE_TIME * prt, BSTR * ppName) = 0;
    STDMETHOD(ChapSet)(DWORD iIndex, REFERENCE_TIME rt, LPCWSTR pName) = 0;
    STDMETHOD(ChapAppend)(REFERENCE_TIME rt, LPCWSTR pName) = 0;
    STDMETHOD(ChapRemoveAt)(DWORD iIndex) = 0;
    STDMETHOD(ChapRemoveAll)() = 0;
    STDMETHOD_(long, ChapLookup)(REFERENCE_TIME * prt, BSTR * ppName) = 0;
    STDMETHOD(ChapSort)() = 0;
};
#endif