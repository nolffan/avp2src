// wonptrq503.h -- the engine's view of WONAPI's reference-counted smart
// pointer (WONCommon/SmartPtr.h's RefCount, ConstSmartPtr<T>, SmartPtr<T>)
// and the two AuthContext pointers at file scope in clientmgr2.cpp (names
// ours, the layout WONAPI's): clientmgr3.cpp's peer authentication and
// shell setup use the first, nothing uses the second.

#ifndef __WONPTRQ503_H__
#define __WONPTRQ503_H__

extern "C" {
__declspec(dllimport) long __stdcall InterlockedIncrement(long* pAddend);
__declspec(dllimport) long __stdcall InterlockedDecrement(long* pAddend);
}

class CWonRefCountQ503                       // WONAPI::RefCount
{
protected:
	virtual ~CWonRefCountQ503() {}

public:
	const CWonRefCountQ503* CreateRef() const
	{
		InterlockedIncrement(&mRefCount);
		return this;
	}

	void Release()
	{
		if (InterlockedDecrement(&mRefCount) <= 0)
			delete this;
	}

private:
	mutable long mRefCount;                  // +0x04
};

template<class T>
class CWonConstSmartPtrQ503                  // WONAPI::ConstSmartPtr<T>
{
protected:
	T* mObject;

public:
	CWonConstSmartPtrQ503() : mObject(LTNULL) {}
	~CWonConstSmartPtrQ503() { if (mObject != LTNULL) mObject->Release(); }

	const T* operator->() const { return mObject; }
	operator const T*() const { return mObject; }
	const T* get() const { return mObject; }

	const T* operator=(const T* thePtr)
	{
		if (mObject != thePtr)
		{
			if (mObject != LTNULL) mObject->Release();
			mObject = (T*)(thePtr ? thePtr->CreateRef() : LTNULL);
		}
		return thePtr;
	}

	const CWonConstSmartPtrQ503& operator=(const CWonConstSmartPtrQ503& theCopy)
	{
		operator=(theCopy.get());
		return *this;
	}
};

template<class T>
class CWonSmartPtrQ503 : public CWonConstSmartPtrQ503<T>  // WONAPI::SmartPtr<T>
{
public:
	CWonSmartPtrQ503() {}

	T* operator=(T* thePtr)
	{
		CWonConstSmartPtrQ503<T>::operator=(thePtr);
		return thePtr;
	}

	T* operator->() const { return mObject; }
	operator T*() const { return mObject; }
	T* get() const { return mObject; }
};

// AVP2: what the pointer holds -- the game shell's GetAuthContext (slot 1,
// `call *0x4(%eax)` at Primal Hunt 0x00410e65) hands back a
// WONAuth/AuthContext.h AuthContext, and the peer-auth start asks it for
// its peer data (GetPeerData, 0x004abe40, an AuthPeerDataPtr by value).
class CWonAuthPeerDataQ548 : public CWonRefCountQ503      // WONAPI::AuthPeerData
{
};

class CWonAuthContextQ548 : public CWonRefCountQ503       // WONAPI::AuthContext
{
public:
	CWonConstSmartPtrQ503<CWonAuthPeerDataQ548> GetPeerData();  // PH 0x004abe40
};

extern CWonSmartPtrQ503<CWonAuthContextQ548> g_WonPtrQ503;        // PH 0x004f0118, AvP2 0x004def68
extern CWonSmartPtrQ503<CWonAuthContextQ548> g_WonPtrUnusedQ503;  // PH 0x004f011c, AvP2 0x004def6c

#endif
