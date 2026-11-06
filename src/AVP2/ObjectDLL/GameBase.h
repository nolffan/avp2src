// ----------------------------------------------------------------------- //
//
// MODULE  : GameBase.h
//
// PURPOSE : Game base object class definition
//
// CREATED : 10/8/99
//
// (c) 1999 Monolith Productions, Inc.  All Rights Reserved
//
// ----------------------------------------------------------------------- //

#ifndef __GAME_BASE_H__
#define __GAME_BASE_H__

// ----------------------------------------------------------------------- //

#include "ltengineobjects.h"
#include "iobjectplugin.h"

// ----------------------------------------------------------------------- //

class GameBase : public BaseClass
{
	public:

		GameBase(uint8 nType = OT_NORMAL);
		virtual ~GameBase();

		virtual void PostStartWorld(uint8 nLoadGameFlags);

#ifdef PRIMAL_HUNT
		// PH: Third Law's engine-message handlers, vtable slots 4 to 13 of
		// every GameBase (PHObject.lto's GameBase vtable, 0x10289948), which
		// GameBase::EngineMessageFn (0x100ecc50) dispatches to and
		// GameBase::ObjectMessageFn (0x100ec670) sends MID_TRIGGER to.
		// OnTouchNotify and OnModelStringKey (0x101518b0, 0x101524a0) are
		// defined in GameBase.cpp, not inline: an inline body is one more
		// declaration unit in every file that includes this header, and that
		// count (mod 64) flips VolumeBrush's class initializer (0x10139330)
		// and CinematicTrigger's (0x100c4da0) -- tools/guides/09, "The
		// declaration count is a symbol count"
		virtual uint32		OnPrecreate(void *pData, uint32 nInfo);			// 0x100ed290
		virtual uint32		OnInitialUpdate(uint32 nInfo);					// 0x100ed2e0
		virtual uint32		OnActivating();									// 0x100ed350
		virtual uint32		OnDeactivating();								// 0x100ed390
		virtual uint32		OnUpdate(LTFLOAT fFrameTime);					// 0x100ed420
		virtual uint32		OnTrigger(HOBJECT hSender, const char* szMsg);	// 0x100ed430
		virtual uint32		OnTouchNotify(void *pData, LTFLOAT fData);
		virtual uint32		OnModelStringKey(void *pData);
		virtual void		OnSave(HMESSAGEWRITE hWrite);					// 0x100ed450
		virtual void		OnLoad(HMESSAGEREAD hRead);						// 0x100ed460
#endif

#ifndef _FINAL
		virtual void		CreateBoundingBox();
		virtual void		RemoveBoundingBox();
		virtual void		UpdateBoundingBox();
#endif
		virtual LTBOOL		CanShootThrough() { return LTFALSE; }
		virtual LTBOOL		CanSeeThrough()   { return LTFALSE; }

        uint32 ObjectMessageFn(HOBJECT hSender, uint32 messageID, HMESSAGEREAD hRead);

		LTFLOAT GetUpdateDelta() const { return m_fUpdateDelta; }


		// Used to tell the object to move to floor on the next MID_UPDATE
		void				SetMoveToFloor( LTBOOL bMoveToFloor ) { m_bMoveToFloor = bMoveToFloor; }
		void				SetTeleportToFloor( LTBOOL bMoveToFloor ) { m_bTeleportToFloor = bMoveToFloor; }
		LTBOOL				GetMoveToFloor( ) { return m_bMoveToFloor; }

	protected:

		// Handles engine messages
		virtual uint32		EngineMessageFn(uint32 messageID, void *pData, LTFLOAT lData);
		virtual LTBOOL		AllowInGameType();

        LTBOOL	ReadProp(ObjectCreateStruct *pData);
#ifndef _FINAL
		virtual	LTVector	GetBoundingBoxColor();
#endif
		virtual void SetNextUpdate(LTFLOAT fDelta, LTFLOAT fDeactivateTime = 0.0f);

	private:

		void				TriggerMsg(HOBJECT hSender, const char* pMsg);
		void				Save(HMESSAGEWRITE hWrite);
		void				Load(HMESSAGEREAD hRead);
		void				Update( );

	protected:

		HSTRING		m_szName;

		uint32		m_dwGameTypes;
		uint32		m_dwDifficulty;
		uint32		m_dwClassWeapons;

		HOBJECT		m_hDimsBox;
		LTVector	m_vColor;
		LTFLOAT		m_fAlpha;

		LTFLOAT		m_fUpdateDelta;
		LTFLOAT		m_fLastUpdateTime;

        uint32      m_dwOriginalFlags;

		LTBOOL		m_bStartHidden;

	private:

		LTBOOL		m_bMoveToFloor;
		LTBOOL		m_bTeleportToFloor;
};

// ----------------------------------------------------------------------- //
// Plugin class for static string lists

class CGameBasePlugin : public IObjectPlugin
{
  public:

	virtual LTRESULT PreHook_EditStringList(
		const char* szRezPath, 
		const char* szPropName, 
		char* const * aszStrings, 
		uint32* pcStrings, 
		const uint32 cMaxStrings, 
		const uint32 cMaxStringLength);

  protected :

};

extern const LTFLOAT c_fMaxUpdateDelta;

// ----------------------------------------------------------------------- //

#endif  // __GAME_BASE_H__

