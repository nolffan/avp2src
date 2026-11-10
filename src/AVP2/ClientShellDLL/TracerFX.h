// ----------------------------------------------------------------------- //
//
// MODULE  : TracerFX.h
//
// PURPOSE : Tracer special fx class - Definition
//
// CREATED : 1/21/97
//
// ----------------------------------------------------------------------- //

#ifndef __TRACER_FX_H__
#define __TRACER_FX_H__

#include "SpecialFX.h"
#include "PolyLineFX.h"
#include "FXButeMgr.h"

struct TRCREATESTRUCT : public SFXCREATESTRUCT
{
	TRCREATESTRUCT();

    LTVector    vStartPos;
    LTVector	vEndPos;
	TRACERFX*	pTracerFX;
};

inline TRCREATESTRUCT::TRCREATESTRUCT()
{
	vStartPos.Init();
	vEndPos.Init();
    pTracerFX = LTNULL;
}


class CTracerFX : public CSpecialFX
{
	public :

		CTracerFX() : CSpecialFX()
		{
            m_bFirstUpdate  = LTTRUE;
			m_fStartTime	= 0.0f;
			m_fDist			= 0.0f;
			m_fLifetime		= 0.0f;
			m_fVelocity		= 0.0f;
			m_vDir.Init(0, 0, 0);
			m_vTracerPos.Init();
		}

#ifdef PRIMAL_HUNT
		// RETAIL BUG: the constructor leaves m_hFlares unset (Init clears
		// them, but only once it has its TRACERFX), and the destructor's
		// Term removes them: a tracer whose Init failed early removes
		// three garbage handles
		virtual ~CTracerFX()
		{
			Term();
		}

		virtual void Term();
		virtual void OnObjectRemove(HOBJECT hObj);
#endif

        virtual LTBOOL Init(HLOCALOBJ hServObj, HMESSAGEREAD hRead);
        virtual LTBOOL Init(SFXCREATESTRUCT* psfxCreateStruct);
        virtual LTBOOL Update();

	protected :

		TRCREATESTRUCT	m_cs;

        LTBOOL          m_bFirstUpdate;
        LTFLOAT         m_fStartTime;
        LTVector        m_vDir;
        LTFLOAT         m_fDist;
        LTFLOAT         m_fLifetime;
		LTFLOAT			m_fVelocity;
        LTVector        m_vTracerPos;

		CPolyLineFX		m_Tracer;

#ifdef PRIMAL_HUNT
		// Primal Hunt: a single-segment tracer (TRACERFX's bSingleSegment)
		// runs its head and tail out along the path in half a second, with
		// three flare sprites at the head (PHCShell.dll 0x1012da40..
		// 0x1012df10; names ours)
		HOBJECT	CreateFlare(char* szSprite, LTVector & vPos, LTFLOAT fScale, LTBOOL bGlow);
		void	RemoveFlares();
		void	UpdateFlares(LTVector & vPos);
		void	SetFlaresHidden(LTBOOL bHide);

		LTVector		m_vFlareStart;
		LTFLOAT			m_fHeadVelocity;
		LTFLOAT			m_fTailVelocity;
		LTFLOAT			m_fFlareTime;
		HOBJECT			m_hFlares[3];
#endif
};

//-------------------------------------------------------------------------------------------------
// SFX_TracerFactory
//-------------------------------------------------------------------------------------------------
#ifndef C_SPECIAL_FX_FACTORY_H
#include "CSpecialFXFactory.h"
#endif

#ifndef __SFX_MSG_IDS_H__
#include "SFXMsgIds.h"
#endif

class SFX_TracerFactory : public CSpecialFXFactory
{
	SFX_TracerFactory() : CSpecialFXFactory(SFX_TRACER_ID) {;}
	static const SFX_TracerFactory registerThis;

	virtual CSpecialFX* MakeShape() const;
};

#endif // __TRACER_FX_H__