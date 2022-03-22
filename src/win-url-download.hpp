#pragma once
#include <UrlMon.h>
#include <cstdio>
#include <functional>
#include <iostream>
#include <vector>

#pragma comment(lib, "urlmon.lib")
#include <fstream>
#include <tchar.h>

class CBindStatusCallback : public IBindStatusCallback {
    using OnProgressCallBack_T = std::function<void(float)>;
    CBindStatusCallback() {}
    virtual ~CBindStatusCallback() {}

public:
    inline static CBindStatusCallback *GenerateAnInstance() { return new CBindStatusCallback(); }

    void OnProgressCallBack(OnProgressCallBack_T callback) { _OnProgressCallBack = callback; }

private:
    ULONG                m_uRefCount = 1;
    OnProgressCallBack_T _OnProgressCallBack;
    // std::vector<uint8_t> data;

public:
    // std::vector<uint8_t> &GetData() { return data; }
    STDMETHOD_(ULONG, Release)() {
        ULONG uRet = --m_uRefCount;
        if (0 == m_uRefCount)
            delete this;
        return uRet;
    }
    STDMETHOD(OnProgress)
    (ULONG   ulProgress,    //
     ULONG   ulProgressMax, //
     ULONG   ulStatusCode,
     LPCWSTR szStatusText) {
        if (ulProgressMax != 0 && _OnProgressCallBack) {
            _OnProgressCallBack(ulProgress * 100.0 / ulProgressMax);
        }
        return S_OK;
    }

    STDMETHOD(OnDataAvailable)
    (DWORD grfBSCF, DWORD dwSize, FORMATETC __RPC_FAR *pformatetc, STGMEDIUM __RPC_FAR *pstgmed) {
        /*size_t old_size = data.size();
        data.resize(old_size + dwSize);
        ULONG mSize = 0;
        pstgmed->pstm->Read(data.data() + old_size, dwSize, &mSize);
        if (mSize == dwSize)
            return S_OK;
        else
            return S_FALSE;*/
        return S_OK;
    }

    STDMETHOD(OnStartBinding)(DWORD dwReserved, IBinding __RPC_FAR *pib) { return E_NOTIMPL; }
    STDMETHOD(GetPriority)(LONG __RPC_FAR *pnPriority) { return E_NOTIMPL; }
    STDMETHOD(OnLowResource)(DWORD reserved) { return E_NOTIMPL; }
    STDMETHOD(OnStopBinding)(HRESULT hresult, LPCWSTR szError) { return E_NOTIMPL; }
    STDMETHOD(GetBindInfo)(DWORD __RPC_FAR *grfBINDF, BINDINFO __RPC_FAR *pbindinfo) { return E_NOTIMPL; }
    STDMETHOD(OnObjectAvailable)(REFIID riid, IUnknown __RPC_FAR *punk) { return E_NOTIMPL; }
    STDMETHOD(QueryInterface)(REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject) { return E_NOTIMPL; }
    STDMETHOD_(ULONG, AddRef)() { return m_uRefCount++; }
};
