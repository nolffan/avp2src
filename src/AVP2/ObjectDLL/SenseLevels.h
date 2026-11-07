// ----------------------------------------------------------------------- //
//
// MODULE  : SenseLevels.h
//
// PURPOSE : CSenseLevels - how much of each sense an AI has of one object
//
//           Primal Hunt (Third Law). No source shipped: written from
//           PHObject.lto, 0x10155ec0..0x1015604b. The file's, class's and
//           members' names are ours.
//
// ----------------------------------------------------------------------- //

#ifndef __SENSE_LEVELS_H__
#define __SENSE_LEVELS_H__

#ifdef PRIMAL_HUNT

#define NUM_SENSE_LEVELS	3

class CSenseLevels
{
	public :

		CSenseLevels();											// 0x10155ec0

		void	Decay(LTFLOAT fTime);							// 0x10155ef0
		void	Add(int nSense, LTFLOAT fAmount);				// 0x10155f20
		LTBOOL	IsAware(int nSense);							// 0x10155f40
		void	Clear();										// 0x10155f70
		void	Save(HMESSAGEWRITE hWrite);						// 0x10155f90
		LTBOOL	HasAny();										// 0x10155fd0
		void	SetRange(int nSense, LTFLOAT fRange);			// 0x10156000
		void	SetDecay(int nSense, LTFLOAT fDecay);			// 0x10156020
		LTFLOAT	GetRange(int nSense);							// 0x10156040

	protected :

		struct Sense
		{
			LTFLOAT	m_fLevel;
			LTFLOAT	m_fRange;		// aware at this level
			LTFLOAT	m_fDecay;		// per second
		};

		Sense	m_Senses[NUM_SENSE_LEVELS];
};

#endif // PRIMAL_HUNT

#endif // __SENSE_LEVELS_H__
